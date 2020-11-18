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

#ifdef ENABLE_DAC
 typedef lamb::device::pt8211                               dac;
#endif

 typedef lamb::ring_buffer<voices::sample, 256>             draw_buffer;
 typedef controls::application_event                        application_event;
 typedef controls::application_event_type                   application_event_type;

#ifdef ENABLE_TFT
 static constexpr  uint32_t             TFT_DC              = PA8;
 static constexpr  uint32_t             TFT_CS              = PB12;
#endif
 
#ifdef ENABLE_DAC
 static constexpr  uint32_t             I2S_WS              = PA15;
#endif
 
 static constexpr  uint32_t             SD_CS               = PB1;

private:
 
 static            int32_t              _avg_sample;
 static            size_t               _sample_ix0;
 static            size_t               _sample_ix1;
 static            HardwareTimer        _timer_1; 
 static            HardwareTimer        _timer_2;
 static            HardwareTimer        _timer_3;

#ifdef ENABLE_DAC
 static            dac                  _dac;
#endif

#ifdef ENABLE_TFT
 static            tft                  _tft;
#endif
 
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
   
   if (_drawn && (_value == newval)) {
    // Serial.println("Abort.");

    return;
   }

   _value   = newval;

   if (! _drawn) {
#ifdef ENABLE_TFT
    application::_tft.setCursor(_x_pos, _y_pos);
    application::_tft.setTextColor(_color);
    application::_tft.setTextSize(2);
    
    application::_tft.print(_name);
#endif
    _drawn = true;
   }

   uint16_t x_pos = _x_pos + 11 * _name_len;
   
#ifdef ENABLE_TFT
   application::_tft.setCursor(x_pos, _y_pos);
#endif

   uint16_t font_red = 0, font_green = 0;
   if (_value >= 56) {
    font_red   = (_value - 56) << 2;
    font_green = (_value - 56) << 3;
   }
   else if (_value <= 8) {
    font_red   = (8 - _value) << 2;
    font_green = (8 - _value) << 3;
   }
   
   uint16_t font_color = (font_red << 11) | (font_green << 5);
   
#ifdef ENABLE_TFT
   application::_tft.setTextColor(font_color);
   application::_tft.setTextSize(2);
#endif

   uint16_t red_value   = _value >> 1;
   uint16_t green_value = 63 - _value;
   uint16_t color = (red_value << 11) | (green_value << 5);
   
#ifdef ENABLE_TFT
   application::_tft.fillRect (
    x_pos, _y_pos - 2,
    _value, 18,
    color
   );
   
   application::_tft.fillRect (x_pos + _value, _y_pos - 2, _width - _value, 18, ILI9341_BLACK);

   application::_tft.print(_value);
#endif
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

#ifdef ENABLE_TFT
 static            void                 setup_tft();
#endif
 
 static            void                 setup_dac();
 static            void                 setup_timers();
 static            void                 setup_sd();
 static            void                 remap_spi1();
 
public:
 static            void                 setup();
 static            void                 loop();
};

#endif

/* Local Variables:  */
/* fill-column: 100  */
/* End:              */
