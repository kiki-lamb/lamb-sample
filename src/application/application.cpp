#include "application/application.h"
#include <inttypes.h>
#include <Arduino.h>
#include <math.h>
#include "SD.h"

using namespace lamb;

using namespace lamb::tables;

//////////////////////////////////////////////////////////////////////////////
SPIClass                     application::_spi_1      { 1 };
SPIClass                     application::_spi_2      { 2 };
size_t                       application::_sample_ix0 { 0 };
HardwareTimer                application::_timer_1    { 1 };
HardwareTimer                application::_timer_2    { 2 };
HardwareTimer                application::_timer_3    { 3 };
application::draw_buffer     application::_draw_buffer{   };

#ifndef DISABLE_DAC
application::dac             application::_dac{ application::DAC_WS };
#endif

application::tft             application::_tft{ application::TFT_CS, application::TFT_DC };

application::displayed_value<voices::filter::unsigned_internal_t::value_type>
application::_displayed_filter_freq{ "Freq: ", 184, 5,  10, 64 };

application::displayed_value<voices::filter::unsigned_internal_t::value_type>
application::_displayed_filter_res { "Res:  ", 184, 30, 10, 64 };

application::displayed_value<u0q16::value_type>
application::_displayed_vol        { "Vol:  ", 184, 55, 10, 64 };

////////////////////////////////////////////////////////////////////////////////

bool application::draw_graph() {
 constexpr uint16_t width    = 172;
 static bool        drew_box = false;
 
 if (! drew_box) {
  _tft.drawFastHLine(0,     128, width, ILI9341_GREEN);
  _tft.drawFastVLine(width, 0,   128,   ILI9341_GREEN);

  return drew_box = true;
 }

 if (_draw_buffer.count() < 16)
  return false;
 
 voices::mix tmp { 0 };
 
 tmp  += _draw_buffer.dequeue();
 tmp  += _draw_buffer.dequeue();
 tmp  += _draw_buffer.dequeue();
 tmp  += _draw_buffer.dequeue();
 
 tmp  += _draw_buffer.dequeue();
 tmp  += _draw_buffer.dequeue();
 tmp  += _draw_buffer.dequeue();
 tmp  += _draw_buffer.dequeue();
 
 tmp  += _draw_buffer.dequeue();
 tmp  += _draw_buffer.dequeue();
 tmp  += _draw_buffer.dequeue();
 tmp  += _draw_buffer.dequeue();
 
 tmp  += _draw_buffer.dequeue();
 tmp  += _draw_buffer.dequeue();
 tmp  += _draw_buffer.dequeue();
 tmp  += _draw_buffer.dequeue();
 
 tmp >>= 13; // to 7 bit
 
 uint16_t wave_color = ILI9341_YELLOW;
 
 if (tmp > 62) {
  tmp.value = 62;
 }
 else if (tmp < -63) {
  tmp.value = -63;
 }
 
 static uint16_t       col     = 0;
 uint16_t              tmp_col = col % width;
 
 _tft.drawFastVLine(tmp_col, 1, 128 - 2, ILI9341_BLACK);
 
 if (tmp > 0)
  _tft.drawFastVLine(
   tmp_col,
   64,
   tmp.value,
   wave_color
  );
 else if (tmp < 0) {
  _tft.drawFastVLine(
   tmp_col,
   64 + tmp.value,
   abs(tmp.value),
   wave_color
  );
 }

 col ++;
 
 _tft.drawFastHLine(0,     64,  width, ILI9341_RED);
  
 return true; 
}

//////////////////////////////////////////////////////////////////////////////

void application::draw_time() {
  _tft.setCursor(10, 210);
  _tft.fillRect(10, 210 - 2, 100, 20, ILI9341_BLACK);
  _tft.setTextColor(ILI9341_GREEN);
  _tft.setTextSize(2);
  _tft.print(_sample_ix0 >> 15);
}

//////////////////////////////////////////////////////////////////////////////

void application::k_rate() {
 gpio_write_bit(GPIOC, 13, voices::item(0).state);
  
#ifdef DISABLE_CONTROLS
 return;
#endif

 ::controls::poll();

 while(::controls::queue_count() > 0)
 {
  application_event ae = ::controls::dequeue_event();

#ifdef LOG_EVENT_TIMES
  Serial.print("Dequeue evt ");
  Serial.print(ae.type);
  Serial.print(", ");
  Serial.print(::controls::queue_count());
  Serial.print(" remain.");
  Serial.println();
#endif
  
  switch (ae.type) {
  case application_event_type::EVT_VOLUME:
  {
   
   voices::volume(voices::volume_type(ae.parameter << 4));

   // Serial.print("Vol: ");
   // Serial.print(float(voices::volume()));   
   // Serial.print("Read ");
   // Serial.print(analogRead(PA3));
   // Serial.print(" ");
   // Serial.print(analogRead(PA4));
   // Serial.print(" ");
   // Serial.print(analogRead(PA5));
   // Serial.print(" ");
   // Serial.print(analogRead(PA6));
   // Serial.print(" ");
//   Serial.println();
   break;
  }
  case application_event_type::EVT_TRIGGER:
  {
   voices::trigger(ae.parameter);
      
   break;
  }
  case application_event_type::EVT_PITCH_1:
  {
   voices::pitch(3, ae.parameter);
      
   break;     
  }
  case application_event_type::EVT_PITCH_2:
  {
   voices::pitch(4, ae.parameter);
      
   break;     
  }
  case application_event_type::EVT_PITCH_3:
  {
   voices::pitch(5, ae.parameter);
      
   break;     
  }
  case application_event_type::EVT_FILTER_F_1:
  {
   lamb::u0q16 parameter(ae.parameter << 4); // 12 sig bits to 16

   // Serial.print("F: ");
   // Serial.println(parameter.value);
   
   voices::filter_f(parameter);
     
   break;     
  }
  case application_event_type::EVT_FILTER_Q_1:
  {
   lamb::u0q16 parameter(ae.parameter << 4); // 12 sig bits to 16
     
   voices::filter_q(parameter);
     
   break;     
  }
  default:
  {
#ifdef LOG_UNRECOGNIZED_EVENTS
   Serial.print(F("Unrecognized event: "));
   Serial.print(ae.type);
   Serial.println();
#endif
  }
  }
 }
}

////////////////////////////////////////////////////////////////////////////////

void application::s_rate() {
 voices::mix s = voices::read();

 if (_draw_buffer.writable()) {
  _draw_buffer.enqueue(s);
 }

#ifndef DISABLE_DAC
 _dac.write_mono(s);
#endif

 _sample_ix0 ++;
}
 
////////////////////////////////////////////////////////////////////////////////
      
void application::setup_tft() {
  Serial.println("[Setup] Setup TFT...");

#if TFT_SPI == SPI1
  _tft.begin(_spi_1);
#else
  _tft.begin(_spi_2);
#endif
  
 _tft.setRotation(3);
 _tft.setTextColor(ILI9341_WHITE);  
 _tft.setTextSize(2);
 _tft.fillScreen(ILI9341_BLACK);
}

////////////////////////////////////////////////////////////////////////////////


void application::setup_dac() {
#ifndef DISABLE_DAC
 Serial.println("[Setup] Setup DAC...");

#if DAC_SPI == SPI1
 _dac.setup(_spi_1);
#else
 _dac.setup(_spi_2);
#endif
#endif
}

////////////////////////////////////////////////////////////////////////////////

void application::setup_timers() {
 Serial.println("[Setup] Setup timers...");

 device::maple_timer::setup(_timer_1, voices::S_RATE,     s_rate);
 device::maple_timer::setup(_timer_2, ::controls::K_RATE, k_rate);
}

////////////////////////////////////////////////////////////////////////////////

void print_directory(File dir, int numTabs = 0, bool recurse = false) {
 while (true) {
  File entry =  dir.openNextFile();

  if (! entry) {
   // no more files
   break;
  }
    
  for (uint8_t i = 0; i < numTabs; i ++) {
   Serial.print('\t');
  }
    
  Serial.print(entry.name());

  if (entry.isDirectory()) {
   Serial.println("/");
   if (recurse) {
    print_directory(entry, numTabs + 1);
   }
  } else {
   Serial.print("\t\t");
   Serial.println(entry.size(), DEC);
  }

  entry.close();
 }
}

//////////////////////////////////////////////////////////////////////////////////////////

void application::setup_spis() {
#ifndef REMAP_SPI1 // not remapped
 gpio_set_mode (GPIOA,  4, GPIO_AF_OUTPUT_PP);   // SPI1 NSS
 gpio_set_mode (GPIOA,  5, GPIO_AF_OUTPUT_PP);   // SPI1 SCK
 gpio_set_mode (GPIOA,  6, GPIO_INPUT_FLOATING); // SPI1 MISO
 gpio_set_mode (GPIOA,  7, GPIO_AF_OUTPUT_PP);   // SPI1 MOSI
#else // remapped
 Serial.println("[Setup] Remap SPI1...");  
 
 afio_remap (AFIO_REMAP_SPI1);
 
 gpio_set_mode (GPIOA, 15, GPIO_AF_OUTPUT_PP);   // Alt SPI1 NSS
 gpio_set_mode (GPIOB,  3, GPIO_AF_OUTPUT_PP);   // Alt SPI1 SCK
 gpio_set_mode (GPIOB,  4, GPIO_INPUT_FLOATING); // Alt SPI1 MISO
 gpio_set_mode (GPIOB,  5, GPIO_AF_OUTPUT_PP);   // Alt SPI1 MOSI
#endif
 
 Serial.println("[Setup] Start SPI1...");
 
 _spi_1.begin();
 
 Serial.println("[Setup] Start SPI2...");
 
 gpio_set_mode (GPIOB, 12, GPIO_AF_OUTPUT_PP);   // SPI2 NSS
 gpio_set_mode (GPIOB, 13, GPIO_AF_OUTPUT_PP);   // SPI2 SCK
 gpio_set_mode (GPIOB, 14, GPIO_INPUT_FLOATING); // SPI2 MISO
 gpio_set_mode (GPIOB, 15, GPIO_AF_OUTPUT_PP);   // SPI2 MOSI
 
 _spi_2.begin();
}

//////////////////////////////////////////////////////////////////////////////////////////

void application::setup_sd() {
#ifndef DISABLE_SD
 Serial.println("[Setup] Setup SD card...");

 if (SD.begin(SD_CS)) {
  Serial.println("[Setup] Successfully setup SD card.");
 }
 else {
  Serial.println("[Setup] Failed to setup SD card.");
 }
#endif
}

//////////////////////////////////////////////////////////////////////////////////////////

void application::setup() {
 delay(5000);

 gpio_set_mode(GPIOC, 13, GPIO_OUTPUT_PP); // builtin LED
 
 Serial.begin(64000000);
 Serial.println("[Setup] Begin...");

 voices::setup();

#ifndef DISABLE_CONTROLS
 ::controls::setup();
#endif

 setup_spis();

 setup_sd();

 setup_tft();

 setup_dac();

 #ifdef REMAP_SPI1
   Serial.println("[Setup] Correct PA5 pin mode...");
   pinMode(PA5, INPUT);
 #endif
  
 setup_timers();

 Serial.println("[Setup] Complete.");
}
  
////////////////////////////////////////////////////////////////////////////////

bool application::sixteenth_second() {
 constexpr uint32_t SIXTEENTH_SECOND      = voices::S_RATE >> 4;
 static uint32_t    last_sixteenth_second = 0;
 
 if ((uint32_t)((_sample_ix0 - last_sixteenth_second)) < SIXTEENTH_SECOND)
  return false;
 
 last_sixteenth_second = _sample_ix0;
 
 _displayed_filter_freq.update(voices::filter_f().value);
 _displayed_filter_res .update(voices::filter_q().value);
 _displayed_vol        .update(voices::volume().value);
 
 return true;
}

////////////////////////////////////////////////////////////////////////////////

bool application::half_second() {
 constexpr uint32_t HALF_SECOND      = voices::S_RATE >> 1;
 static uint32_t    last_half_second = 0;
 
 if ((uint32_t)((_sample_ix0 - last_half_second)) < HALF_SECOND)
  return false;
 
 draw_time();
 
 last_half_second = _sample_ix0;

#ifndef DISABLE_SD
 File root = SD.open("/");
 
 print_directory(root);
 
 root.close();
#endif
 
#ifdef TEST_TRIGGER
  voices::trigger(0);
#endif

  return true;
}

////////////////////////////////////////////////////////////////////////////////

#ifdef LOG_DRAW_RATES
u16q16   draw_operations{ 0 };
#endif

bool application::one_second() {
 constexpr uint32_t ONE_SECOND           = voices::S_RATE;
 static uint32_t    last_one_second      = 0;

 if ((uint32_t)((_sample_ix0 - last_one_second)) < voices::S_RATE)
  return false;

 last_one_second                         = _sample_ix0;
 
#ifdef LOG_DRAW_RATES
 static u16q16      avg_draw_operations  { 0 };
 static uint32_t    seconds              = 0;
 seconds                                += 1;
 avg_draw_operations                    *= u16q16(0xe000);
 avg_draw_operations                    += draw_operations * u16q16(0x1fff);
 
 Serial.print(seconds);
 Serial.print(F(", "));
 Serial.print(float(draw_operations));
 Serial.print(F(", "));
 Serial.print(float(avg_draw_operations));
 Serial.print(F(" avg, "));
 Serial.print(_draw_buffer.count());
 Serial.println();
 
 draw_operations.value                   = 0;
#endif
 
 return true;
}

////////////////////////////////////////////////////////////////////////////////

bool application::idle() {
 if (! draw_graph())
  return true;
 
 draw_operations += u16q16(1, 0);

 return true;
}

////////////////////////////////////////////////////////////////////////////////

void application::loop() {
 (
  one_second()       ||
  half_second()      ||
  sixteenth_second() ||
  idle()
 );
}

////////////////////////////////////////////////////////////////////////////////

/* Local Variables:  */
/* fill-column: 100  */
/* End:              */
