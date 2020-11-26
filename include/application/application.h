#ifndef APPLICATION_H
#define APPLICATION_H

#include <inttypes.h>
#include <Arduino.h>
#include "lamb.h"
#include "controls/controls.h"
#include "voices/voices.h"
#include "events/control.h"
#include "events/application.h"

#ifdef USE_GENERIC_ILI9341
#include "Adafruit_ILI9341_STM.h"
#endif

class application {

public:
#ifdef USE_GENERIC_ILI9341
 typedef Adafruit_ILI9341_STM                               tft;
#else
 typedef lamb::device::Adafruit_ILI9341_STM32F1             tft;
#endif
 typedef lamb::ring_buffer<voices::sample, 256>             draw_buffer;
 typedef controls::application_event                        application_event;
 typedef controls::application_event_type                   application_event_type;
  
 static constexpr  uint32_t             TFT_DC              = PB0;
 static constexpr  uint32_t             TFT_CS              = PA4;
 static constexpr  uint32_t             I2S_WS              = PA15;

private:
 static            SPIClass             _spi_1;
 static            SPIClass             _spi_2;
 static            int32_t              _avg_sample;
 static            size_t               _sample_ix0;
 static            size_t               _sample_ix1;
 static            HardwareTimer        _timer_1; 
 static            HardwareTimer        _timer_2;
 static            HardwareTimer        _timer_3;
 static            tft                  _tft;
 static            draw_buffer          _draw_buffer;

 static            void                 k_rate();
 static            void                 s_rate();
 static            void                 setup_tft();
 static            void                 setup_timers();
 
public:
 static            void                 setup();
 static            void                 loop();
};

#endif

/* Local Variables:  */
/* fill-column: 100  */
/* End:              */
