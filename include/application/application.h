#ifndef APPLICATION_H
#define APPLICATION_H

#include <inttypes.h>
#include <Arduino.h>
#include "Adafruit_ILI9341_STM.h"
#include "lamb.h"

class application {

 static            Adafruit_ILI9341_STM _tft;
 static            SPIClass             _spi_1;
 static            HardwareTimer        _timer_1; 

 static            void                 sample_rate();
 
public:
 static            void                 setup();
 static            void                 loop();
};

#endif

/* Local Variables:  */
/* fill-column: 100  */
/* End:              */
