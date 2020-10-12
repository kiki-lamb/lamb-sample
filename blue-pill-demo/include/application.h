#ifndef APPLICATION_H
#define APPLICATION_H

#include <inttypes.h>
#include "samples.h"
#include "lamb.h"

class application {
public:
  typedef lamb::oneshot_plus sample_t;

  static const uint32_t             K_RATE;
  static const uint32_t             S_RATE;
  static const uint32_t             TFT_DC        = PA8;
  static const uint32_t             TFT_CS        = PB12;
  static const uint32_t             I2S_WS        = PA3;
  static const uint32_t             I2S_BCK       = PA5;
  static const uint32_t             I2S_DATA      = PA7;
  static const uint32_t             CAPTURE_RATIO = 3;
  static const size_t               BLOCK_SIZE    = Samples::NUM_ELEMENTS / 6;
  static const uint32_t             V_SPACING     = 48;  

  static       uint16_t             knob0;
  static       uint16_t             knob1;
  static       uint16_t             knob2;
  static       int32_t              avg_sample;
  static       size_t               sample_ix;
  static       size_t               total_samples;
  static       double               pct;
  static       HardwareTimer        timer_1; 
  static       HardwareTimer        timer_2;
  static       HardwareTimer        timer_3;
  static       lamb::device::pt8211 pt8211;
  static       volatile bool        draw_flag;
  static       sample_t *           voices[6];
  static       uint8_t              last_button_values;
  static       uint8_t              queued; 

  static       lamb::device::Adafruit_ILI9341_STM_SPI2  tft;
  static       lamb::ring_buffer<int16_t, 256>          drawbuff;
 
  static       void                 k_rate();
  static       void                 s_rate();
  static       void                 graph();  
  static       void                 draw_text();
  static       void                 setup();
  static       void                 loop();

};

#endif
