#ifndef APPLICATION_H
#define APPLICATION_H

#include <inttypes.h>
#include <Arduino.h>
#include "Adafruit_ILI9341_STM.h"

class application {

 static            Adafruit_ILI9341_STM tft;
 static            SPIClass             spi_1;
 static            HardwareTimer        tim_1; 

 static            void                 sample_rate();
 
public:
 static            void                 setup();
 static            void                 loop();
};

#endif
