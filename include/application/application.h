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
 typedef lamb::ring_buffer<voices::sample, 256>             draw_buffer;
 typedef controls::application_event                        application_event;
 typedef controls::application_event_type                   application_event_type;
  
 static const      uint32_t             TFT_DC              = PA8;
 static const      uint32_t             TFT_CS              = PB12;

 static const      uint32_t             I2S_WS              = PA15;
 static const      uint32_t             I2S_BCK             = PA5;
 static const      uint32_t             I2S_DATA            = PA7;

 static const      uint32_t             K_RATE;
 static const      uint32_t             CAPTURE_RATIO;
 static const      uint32_t             V_SPACING;

private:
 
 static            int32_t              _avg_sample;
 static            size_t               _sample_ix0;
 static            size_t               _sample_ix1;
 static            HardwareTimer        _timer_1; 
 static            HardwareTimer        _timer_2;
 static            HardwareTimer        _timer_3;
 static            dac                  _dac;
 static            tft                  _tft;
 static            draw_buffer          _draw_buffer;

 template <typename value_t_>
 class displayed_value {

 public:
  typedef value_t_ value_t;
 
 private:
  value_t       _value;
//  bool          _flagged;
  char const *  _name;
  uint8_t       _name_len;
  uint16_t      _x_pos;
  uint16_t      _y_pos;
  uint8_t       _downshift;
  uint16_t      _width;
  uint32_t      _color;
  bool          _drawn;
  
 public:
  displayed_value(
   char     const * name,
   uint16_t const & x_pos,
   uint16_t const & y_pos,
   uint8_t  const & downshift,
   uint16_t const & width = 50,
   uint32_t const & color = ILI9341_GREEN) :
   _name(name),
   _name_len(strlen(name)),
   _x_pos(x_pos),
   _y_pos(y_pos),
   _downshift(downshift),
   _width(width),
   _color(color),
   _drawn(false) {}
  
  void update(value_t newval) {
   newval >>= _downshift;
   
   if (_value == newval) {
    // Serial.println("Abort.");

    return;
   }

   // Serial.println("Draw.");
   
   _value   = newval;

   if (! _drawn) {
    application::_tft.setCursor(_x_pos, _y_pos);
    application::_tft.setTextColor(_color);
    application::_tft.setTextSize(2);
    
    application::_tft.print(_name);

    _drawn = true;
   }

   uint16_t x_pos = _x_pos + 11 * _name_len;
   
   application::_tft.setCursor(x_pos, _y_pos);

   uint16_t text_red   = (64 - _value) >> 1;
   uint16_t text_green = (64 - _value) >> 0;
   uint16_t text_blue  = (64 - _value) >> 1;
   uint16_t text_color = (text_red << 11) | (text_green << 5) | text_blue;
   
   application::_tft.setTextColor( text_color );
   application::_tft.setTextSize(2);

   uint8_t red_value  = _value > 32 ? _value - 32 : 0;
   uint8_t blue_value = 32 - (_value >> 1);
   
   application::_tft.fillRect (
    x_pos, _y_pos - 2,
    _value, 18,
    (red_value << 11) | blue_value
   );
   
   application::_tft.fillRect (x_pos + _value, _y_pos - 2, _width - _value, 18, ILI9341_BLACK);

   application::_tft.print(_value);
  }
 };

 static            displayed_value<voices::filter::unsigned_internal_t::value_type>
 _displayed_filter_freq;

 static            displayed_value<voices::filter::unsigned_internal_t::value_type>
 _displayed_filter_res;
  
 static            displayed_value<lamb::u0q16::value_type>
 _displayed_vol;
  
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
