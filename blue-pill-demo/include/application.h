#ifndef APPLICATION_H
#define APPLICATION_H

#include "lamb.h"
#include "samples.h"
#include "events/control.h"

class application {
public:
  typedef int16_t                                         sample;
  typedef lamb::oneshot_plus                              voice;
  typedef lamb::device::Adafruit_ILI9341_STM_SPI2         tft;
  typedef lamb::device::pt8211                            dac;
  typedef lamb::ring_buffer<int16_t, 256>                 draw_buffer;
  typedef lamb::controls::button                          button;
  typedef events::control                                 control_event;
  typedef events::control_event_type                      control_event_type;
  typedef lamb::events::sources::buffer<control_event, 8> control_source;
  
  typedef lamb::events::sources::button_pad<
    button,
    control_event,
    control_event_type::EVT_BUTTON
    >                                                     button_source;
  
  static const    uint8_t              event_sources_count = 6;
  static const    uint32_t             TFT_DC              = PA8;
  static const    uint32_t             TFT_CS              = PB12;
  static const    uint32_t             I2S_WS              = PA3;
  static const    uint32_t             I2S_BCK             = PA5;
  static const    uint32_t             I2S_DATA            = PA7;
  static const    uint32_t             K_RATE;
  static const    uint32_t             S_RATE;
  
  typedef lamb::events::sources::combine<
    control_event,
    event_sources_count>                                  combined_source;

private:
  static const    uint32_t             CAPTURE_RATIO = 3;
  static const    size_t               BLOCK_SIZE    = Samples::NUM_ELEMENTS / 6;
  static const    uint32_t             V_SPACING     = 48;  

  static          button               _button_device0;
  static          button               _button_device1;
  static          button               _button_device2;
  static          button               _button_device3;
  static          button               _button_device4;
  static          button               _button_device5;
  
  static          button_source        _button_source0;
  static          button_source        _button_source1;
  static          button_source        _button_source2;
  static          button_source        _button_source3;
  static          button_source        _button_source4;
  static          button_source        _button_source5;
  
  static          combined_source      _combined_source;
  
  static          control_source       _control_event_source;
  
  static          uint16_t             _knob0;
  static          uint16_t             _knob1;
  static          uint16_t             _knob2;
  static          int32_t              _avg_sample;
  static          size_t               _sample_ix;
  static          size_t               _total_samples;
  static          double               _pct;
  static          HardwareTimer        _timer_1; 
  static          HardwareTimer        _timer_2;
  static          HardwareTimer        _timer_3;
  static          dac                  _dac;
  static          voice *              _voices[6];
  static          uint8_t              _last_button_values;
  static          uint8_t              _queued; 
  static          tft                  _tft;
  static          draw_buffer          _draw_buffer;
 
  static          void                 k_rate();
  static          void                 s_rate();
  static          void                 graph();  
  static          void                 setup_controls();
  static          void                 setup_voices();
  static          void                 setup_tft();
  static          void                 setup_dac();
  static          void                 setup_timers();
  
public:
  static          void                 setup();
  static          void                 loop();
};

#endif

/* Local Variables:  */
/* fill-column: 100  */
/* End:              */
