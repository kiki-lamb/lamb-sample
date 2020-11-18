#include "application/application.h"
#include <inttypes.h>
#include <Arduino.h>
#include <math.h>
#include "SD.h"

using namespace lamb;

using namespace lamb::tables;

//////////////////////////////////////////////////////////////////////////////

int32_t                      application::_avg_sample        { 0                         };
size_t                       application::_sample_ix0        { 0                         };
size_t                       application::_sample_ix1        { 0                         };
HardwareTimer                application::_timer_1           ( 1                         );
HardwareTimer                application::_timer_2           ( 2                         );
HardwareTimer                application::_timer_3           ( 3                         );
application::dac             application::_dac               ( application::I2S_WS, &SPI );

#ifdef ENABLE_TFT
application::tft             application::_tft(application::TFT_CS, application::TFT_DC  );
#endif

application::draw_buffer     application::_draw_buffer;         

application::displayed_value<voices::filter::unsigned_internal_t::value_type>
application::_displayed_filter_freq("Freq: ", 184, 5,  10, 64);

application::displayed_value<voices::filter::unsigned_internal_t::value_type>
application::_displayed_filter_res ("Res:  ", 184, 30, 10, 64);

application::displayed_value<u0q16::value_type>
application::_displayed_vol ("Vol:  ", 184, 55, 10, 64);

////////////////////////////////////////////////////////////////////////////////

bool application::graph() {
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
  // wave_color = ILI9341_RED;
 }
 else if (tmp < -63) {
  tmp.value = -63;
  // wave_color = ILI9341_RED;
 }
 
 static const uint16_t width = 172;
 static uint16_t       col = 0;
 uint16_t              tmp_col = col % width;
  
#ifdef ENABLE_TFT
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

 _tft.drawFastHLine(0,     64,  width, ILI9341_RED);

 static bool box = false;

 if (! box) {
  _tft.drawFastHLine(0,     128, width, ILI9341_GREEN);
  _tft.drawFastVLine(width, 0,   128,   ILI9341_GREEN);

  box = true;
 }
 
 col ++;
// col %= col_max;

#ifdef TIME_ON_TFT
 static uint32_t last_time = 0;
 uint32_t new_time = millis();
 
 if ((new_time - last_time) > 500) {
  _tft.setCursor(10, 210);
  _tft.fillRect(10, 210 - 2, 100, 20, ILI9341_BLACK);
  _tft.setTextColor(ILI9341_GREEN);
  _tft.setTextSize(2);
  _tft.print(new_time);

  Serial.print("Time: ");
  Serial.println(new_time);

  last_time = new_time;
 } 
#endif

#endif
 return true;
}

//////////////////////////////////////////////////////////////////////////////

void application::k_rate() {
#ifdef DISABLE_CONTROLS
 return;
#endif

 digitalWrite(LED_BUILTIN, voices::item(0).state);
 
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
 // if (_sample_ix0 == (1 << CAPTURE_RATIO)) {
 //  _avg_sample >>= CAPTURE_RATIO;

 //  _sample_ix0 = 0;
 //  _avg_sample = 0;
 // }

 voices::mix s = voices::read();

 if (_draw_buffer.writable()) {
  _draw_buffer.enqueue(s);
 }

 _dac.write_mono(s);

 _sample_ix0  ++;
 _sample_ix1  ++;
}
 
////////////////////////////////////////////////////////////////////////////////

#ifdef ENABLE_TFT
void application::setup_tft() {
  Serial.println("[Setup] Setup TFT...");

 _tft.begin();
 _tft.setRotation(3);
 _tft.setTextColor(ILI9341_WHITE);  
 _tft.setTextSize(2);
 _tft.fillScreen(ILI9341_BLACK);
}
#endif

////////////////////////////////////////////////////////////////////////////////


void application::setup_dac() {
 Serial.println("[Setup] Setup DAC...");

 SPI.begin();
  
 _dac.setup();
}

////////////////////////////////////////////////////////////////////////////////

void application::setup_timers() {
 Serial.println("[Setup] Setup timers...");

 device::maple_timer::setup(_timer_1, voices::S_RATE,     s_rate);
 device::maple_timer::setup(_timer_2, ::controls::K_RATE, k_rate);
}

////////////////////////////////////////////////////////////////////////////////

void print_directory(File dir, uint32 abort_after = 10, int numTabs = 0, bool recurse = false) {
 Serial.println("Listing files...");
 
 while (abort_after-- > 0) {
  File entry =  dir.openNextFile();
  
  if (! entry) {
   // no more files
   entry.close();
   
   break;
  }
    
  for (uint8_t i = 0; i < numTabs; i++) {
   Serial.print('\t');
  }
    
  Serial.print(entry.name());

  if (entry.isDirectory()) {
   Serial.println("/");

   if (recurse) {
    print_directory(entry, abort_after, numTabs + 1);
   }
  } else {
   Serial.print("\t\t");
   Serial.println(entry.size(), DEC);
  }

  entry.close();
 }
}

//////////////////////////////////////////////////////////////////////////////////////////

void application::remap_spi1() {
 Serial.println("[Setup] Remap SPI1...");  
 afio_remap(AFIO_REMAP_USART1);
 afio_cfg_debug_ports (AFIO_DEBUG_SW_ONLY);
 afio_remap (AFIO_REMAP_SPI1);
 gpio_set_mode (GPIOA, 15, GPIO_AF_OUTPUT_PP);
 gpio_set_mode (GPIOB,  3, GPIO_AF_OUTPUT_PP);
 gpio_set_mode (GPIOB,  4, GPIO_INPUT_FLOATING);
 gpio_set_mode (GPIOB,  5, GPIO_AF_OUTPUT_PP);
}

//////////////////////////////////////////////////////////////////////////////////////////

void application::setup_sd() {
  Serial.println("[Setup] Setup SD card...");

 if (SD.begin(SD_CS)) {
  Serial.println("[Setup] Successfully setup SD card.");
 }
 else {
  Serial.println("[Setup] Failed to setup SD card.");
 }
}

//////////////////////////////////////////////////////////////////////////////////////////

void application::setup() {
 delay(5000);

 pinMode(LED_BUILTIN, OUTPUT);
 
 Serial.begin(64000000);
 Serial.println("[Setup] Begin...");

 voices::setup();

#ifndef DISABLE_CONTROLS
 ::controls::setup();
#endif

 remap_spi1();

#ifdef ENABLE_SD
 setup_sd();

 #ifdef SD_TEST_LOOP
 while (true) {
  Serial.print("T: ");
  Serial.println(millis());
  
  File root = SD.open("/");
  
  print_directory(root);

  root.close();
  
  delay(250);
 }
 #endif
#endif
 
#ifdef ENABLE_TFT
 setup_tft();
#endif

 setup_dac();
 
 Serial.println("[Setup] Correct PA5 pin mode...");
 pinMode(PA5, INPUT);

#ifndef DISABLE_TIMERS
 setup_timers();
#endif

 Serial.println("[Setup] Complete.");
}
  
////////////////////////////////////////////////////////////////////////////////

void application::loop() {
 static u16q16   draw_operations                  { 0 };

 static uint32_t last_params_draw                 = 0;

 uint32_t now = millis();

 if ((now - last_params_draw) > 100) {
  last_params_draw = now;

  _displayed_filter_freq.update(voices::filter_f().value);
  _displayed_filter_res .update(voices::filter_q().value);
  _displayed_vol        .update(voices::volume().value);
 } 
 
#ifdef ENABLE_SD
 File root = SD.open("/");

 print_directory(root);

 root.close();
#endif

  if (graph())
   draw_operations += u16q16(1, 0);
  
#ifdef LOG_DRAW_RATES
 if (_sample_ix1 >= voices::S_RATE) {
//  static const uint8_t avging                    = 10;      
  static u16q16        avg_draw_operations       { 0 };
  static uint32_t      tenth_seconds             = 0;
  tenth_seconds                                 += 1;
  _sample_ix1                                    = 0;
  avg_draw_operations                           *= u16q16(0xe000);
  avg_draw_operations                           += draw_operations * u16q16(0x1fff);
    
  Serial.print(tenth_seconds);
  Serial.print(F(", "));
  Serial.print(float(draw_operations));
  Serial.print(F(", "));
  Serial.print(float(avg_draw_operations));
  Serial.print(F(" avg, "));
  // Serial.print(
  //  (avg_draw_operations - tmp_avg_draw_operations).value
  // );
  Serial.print(F(", "));
  Serial.print(_draw_buffer.count());
  Serial.println();
    
  draw_operations.value                          = 0;
 }
#endif
}

////////////////////////////////////////////////////////////////////////////////

/* Local Variables:  */
/* fill-column: 100  */
/* End:              */
