#ifndef APPLICATION_H
#define APPLICATION_H

#include <inttypes.h>
#include <Arduino.h>
#include "lamb.h"
#include "controls/controls.h"
#include "voices/voices.h"
#include "events/control.h"
#include "events/application.h"

class application {
public:
  typedef lamb::device::Adafruit_ILI9341_STM_SPI2            tft;
  typedef lamb::device::pt8211                               dac;
  typedef lamb::ring_buffer<voices::sample, 32>              draw_buffer;
  typedef controls::application_event                        application_event;
  typedef controls::application_event_type                   application_event_type;
  
  static const      uint32_t             TFT_DC              = PA8;
  static const      uint32_t             TFT_CS              = PB12;

  static const      uint32_t             I2S_WS              = PA3;
  static const      uint32_t             I2S_BCK             = PA5;
  static const      uint32_t             I2S_DATA            = PA7;

  static const      uint32_t             K_RATE;
  static const      uint32_t             CAPTURE_RATIO;
  static const      uint32_t             V_SPACING;

private:
  application() = default;
  
  static            int32_t              _avg_sample;
  static            size_t               _sample_ix0;
  static            size_t               _sample_ix1;
  static            HardwareTimer        _timer_1; 
  static            HardwareTimer        _timer_2;
  static            HardwareTimer        _timer_3;
  static            dac                  _dac;
  static            tft                  _tft;
  static            draw_buffer          _draw_buffer;

  
  static            void                 k_rate();
  static            void                 s_rate();
  static            bool                 graph();  
  static            void                 setup_tft();
  static            void                 setup_dac();
  static            void                 setup_timers();
  
public:
  static            void                 setup();
  static            void                 loop();
};

#endif

/* Local Variables:  */
/* fill-column: 100  */
/* End:              */
