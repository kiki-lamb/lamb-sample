#include "application/application.h"
#include <inttypes.h>
#include <Arduino.h>
#include <math.h>
#include "SD.h"

using namespace lamb;

using namespace lamb::tables;

//////////////////////////////////////////////////////////////////////////////
SPIClass                     application::_spi_1             { 1                         };
SPIClass                     application::_spi_2             { 2                         };
HardwareTimer                application::_timer_1           { 1                         };
HardwareTimer                application::_timer_2           { 2                         };
HardwareTimer                application::_timer_3           { 3                         };
application::dac             application::_dac               { application::I2S_WS       };
int32_t                      application::_avg_sample        = 0;
size_t                       application::_sample_ix0        = 0;
size_t                       application::_sample_ix1        = 0;

application::tft             application::_tft(
 application::TFT_CS,
 application::TFT_DC
);

application::draw_buffer     application::_draw_buffer;         

//////////////////////////////////////////////////////////////////////////////

void application::k_rate() {
 ::controls::poll();

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
   lamb::u0q16 parameter (ae.parameter << 4); // 12 sig bits to 16

   // Serial.print("F: ");
   // Serial.println(parameter.value);
   
   voices::filter_f(parameter);
     
   break;     
  }
  case application_event_type::EVT_FILTER_Q_1:
  {
   lamb::u0q16 parameter (ae.parameter << 4); // 12 sig bits to 16
     
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
      
void application::setup_tft() {
 Serial.println("[Setup] Setup TFT...");

 _tft.begin(_spi_1);
 _tft.setRotation(3);
 _tft.setTextColor(ILI9341_WHITE);  
 _tft.setTextSize(2);
 _tft.fillScreen(ILI9341_BLACK);
}

////////////////////////////////////////////////////////////////////////////////


void application::setup_dac() {
 Serial.println("[Setup] Setup DAC...");

 _spi_2.begin();
  
 _dac.setup(_spi_2);
}

////////////////////////////////////////////////////////////////////////////////

void application::setup_timers() {
 Serial.println("[Setup] Setup timers...");

 device::maple_timer::setup(_timer_1, voices::S_RATE,     s_rate);
// device::maple_timer::setup(_timer_2, ::controls::K_RATE, k_rate);

 Serial.println("Done setup timers.");
}

////////////////////////////////////////////////////////////////////////////////

void print_directory(File dir, int numTabs = 0, bool recurse = false) {
 while (true) {
  File entry { dir.openNextFile() };

  if (! entry) {
   // no more files
   break;
  }
    
  for (uint8_t i = 0; i < numTabs; i++) {
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
#ifdef ENABLE_SD
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

 pinMode(LED_BUILTIN, OUTPUT);
 
 Serial.begin(64000000);
 Serial.println("[Setup] Begin...");

 voices::setup();

// ::controls::setup();

// remap_spi1();

 setup_sd();

 setup_tft();
 
 setup_dac();
 
 // Serial.println("[Setup] Correct PA5 pin mode...");
 // pinMode(PA5, INPUT);
 
 setup_timers();

 Serial.println("[Setup] Complete.");
}
  
////////////////////////////////////////////////////////////////////////////////

void application::loop() {
 uint16_t wave_color = ILI9341_YELLOW;
 
 static const uint16_t    width   = 240;
 static uint16_t          col     = 0; 

 uint16_t                 tmp_col = col % width;
 
 static uint32_t          ctr     = 0;
 static uint32_t          accum   = 0;
 static constexpr uint8_t avging  = 12;
 
 uint32_t                 start   = micros(); 
 
 _tft.drawFastVLine(tmp_col, 1, 128 - 2, ILI9341_BLACK);
 
 accum += micros() - start;
 ctr   ++;
 
 if (ctr == (1 << avging)) {
  accum >>= avging;
  
  Serial.print("t = ");
  Serial.print(accum);
  Serial.println();
  
  accum = 0;
  ctr   = 0;
 }
 
 _tft.drawFastVLine(
  tmp_col,
  0,
  tmp_col,
  wave_color
 );
 
 col ++; 
}

////////////////////////////////////////////////////////////////////////////////

/* Local Variables:  */
/* fill-column: 100  */
/* End:              */
