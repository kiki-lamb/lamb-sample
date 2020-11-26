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
int32_t                      application::_avg_sample        = 0;
size_t                       application::_sample_ix0        = 0;
size_t                       application::_sample_ix1        = 0;

application::tft             application::_tft(
 application::TFT_CS,
 application::TFT_DC
);

////////////////////////////////////////////////////////////////////////////////

void application::s_rate() {
 voices::mix s = voices::read();

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

void application::setup_timers() {
 Serial.println("[Setup] Setup timers...");

 device::maple_timer::setup(_timer_1, voices::S_RATE,     s_rate);

 Serial.println("Done setup timers.");
}

//////////////////////////////////////////////////////////////////////////////////////////

void application::setup() {
 delay(5000);

 pinMode(LED_BUILTIN, OUTPUT);
 
 Serial.begin(64000000);
 Serial.println("[Setup] Begin...");

 voices::setup();

 setup_tft();
 
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
