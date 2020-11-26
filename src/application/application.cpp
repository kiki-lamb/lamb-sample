#include "application/application.h"
#include <Arduino.h>

using namespace lamb;

using namespace lamb::tables;

//////////////////////////////////////////////////////////////////////////////

SPIClass                     application::_spi_1    { 1        };
HardwareTimer                application::_timer_1  { 1        };
Adafruit_ILI9341_STM         application::_tft      { PA4, PB0 };

////////////////////////////////////////////////////////////////////////////////

void application::sample_rate() {
 for (size_t ix = 0; ix < 256; ix++)
  asm("NOP");
}
 
//////////////////////////////////////////////////////////////////////////////////////////

void application::setup() {
 Serial.begin(115200);

 _tft.begin(_spi_1);
 _tft.setRotation(3);
 _tft.setTextColor(ILI9341_WHITE);  
 _tft.setTextSize(2);
 _tft.fillScreen(ILI9341_BLACK);

 device::maple_timer::setup(_timer_1, 48000, sample_rate);
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
