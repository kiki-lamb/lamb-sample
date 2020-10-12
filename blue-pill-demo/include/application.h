#ifndef APPLICATION_H
#define APPLICATION_H

#include <inttypes.h>
#include "samples.h"
#include "lamb.h"

class application {
public:
  typedef lamb::oneshot_plus                       voice;
  typedef lamb::device::Adafruit_ILI9341_STM_SPI2  tft;
  typedef lamb::ring_buffer<int16_t, 256>          draw_buff;
  typedef lamb::device::pt8211                     dac;
  
private:
  static const uint32_t             K_RATE;
  static const uint32_t             S_RATE;

public:
  static const uint32_t             TFT_DC        = PA8;
  static const uint32_t             TFT_CS        = PB12;
  static const uint32_t             I2S_WS        = PA3;
  static const uint32_t             I2S_BCK       = PA5;
  static const uint32_t             I2S_DATA      = PA7;

private:
  static const uint32_t             CAPTURE_RATIO = 3;
  static const size_t               BLOCK_SIZE    = Samples::NUM_ELEMENTS / 6;
  static const uint32_t             V_SPACING     = 48;  

  static       uint16_t             _knob0;
  static       uint16_t             _knob1;
  static       uint16_t             _knob2;
  static       int32_t              _avg_sample;
  static       size_t               _sample_ix;
  static       size_t               _total_samples;
  static       double               _pct;
  static       HardwareTimer        _timer_1; 
  static       HardwareTimer        _timer_2;
  static       dac                  _dac;
  static       lamb::flag           _draw_flag;
  static       voice *              _voices[6];
  static       uint8_t              _last_button_values;
  static       tft                  _tft;
  static       draw_buff            _draw_buff;
 
  static       void                 k_rate();
  static       void                 s_rate();
  static       void                 graph();  
  static       void                 draw_text();
public:

  static       void                 setup();
  static       void                 loop();

};

#endif
