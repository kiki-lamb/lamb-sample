#include "application/application.h"
#include <Arduino.h>

using namespace lamb;

using namespace lamb::tables;

//////////////////////////////////////////////////////////////////////////////

SPIClass                     application::spi_1    { 1        };
HardwareTimer                application::tim_1  { 1        };
Adafruit_ILI9341_STM         application::tft      { PA4, PB0 };

////////////////////////////////////////////////////////////////////////////////

void application::sample_rate() {
 for (size_t ix = 0; ix < 256; ix++)
  asm("NOP");
}
 
//////////////////////////////////////////////////////////////////////////////////////////

void application::setup() {
 Serial.begin(115200);

 tft.begin(spi_1);
 tft.setRotation(3);
 tft.setTextColor(ILI9341_WHITE);  
 tft.setTextSize(2);
 tft.fillScreen(ILI9341_BLACK);

 device::maple_timer::setup(tim_1, 48000, sample_rate);
}
  
////////////////////////////////////////////////////////////////////////////////

void application::loop() {
 static const uint16_t    draw_area_width = 240;
 static uint16_t          column          = 0; 

 uint16_t                 tmp_col         = column % draw_area_width;
 
 constexpr uint8_t        fx_shift        = 8;
 constexpr size_t         max_count       = 1 << fx_shift;
 static size_t            counter         = 0;
 static uint32_t          accum           = 0;
 
 uint32_t                 start           = micros(); 
 
 tft.drawFastVLine(column % draw_area_width, 1, 128 - 2, ILI9341_BLACK);
 
 accum   += micros() - start;
 counter ++;
 
 if (counter == (1 << fx_shift)) {
  accum >>= fx_shift;
  
  Serial.print("t = ");
  Serial.print(accum);
  Serial.println();
  
  accum = 0;
  counter   = 0;
 }
 
 tft.drawFastVLine(
  column % draw_area_width,
  0,
  column % draw_area_width,
  ILI9341_YELLOW
 );
 
 column ++; 
}

////////////////////////////////////////////////////////////////////////////////

/* Local Variables:  */
/* fill-column: 100  */
/* End:              */
