#ifndef APPLICATION_H
#define APPLICATION_H

#include <inttypes.h>
#include <Arduino.h>
#include "lamb.h"
#include "controls/controls.h"
#include "voices/voices.h"
#include "events/control.h"
#include "events/application.h"
#include "Adafruit_ILI9341_STM.h"

#define SPI1   1
#define SPI2   2

#ifndef DAC_SPI
#define DAC_SPI SPI2  
#endif

#ifndef TFT_SPI
#define TFT_SPI SPI1
#endif

#ifndef DISABLE_SD
#ifndef SD_SPI
#define SD_SPI TFT_SPI
#endif
#endif

#if (DAC_SPI == TFT_SPI) || (DAC_SPI == SD_SPI)
#error "DAC must not share it's SPI with other devices as it is an I2S device."
#endif

class application {

public:
 typedef Adafruit_ILI9341_STM                               tft;

#ifndef DISABLE_DAC
 typedef lamb::device::pt8211                               dac;
#endif
 
 typedef lamb::ring_buffer<voices::sample, 256>             draw_buffer;
 typedef controls::application_event                        application_event;
 typedef controls::application_event_type                   application_event_type;

#if TFT_SPI == SPI1
#ifndef REMAP_SPI1
 static constexpr  uint32_t            TFT_DC              = PB0; // next to SPI1 MOSI
 static constexpr  uint32_t            TFT_CS              = BOARD_SPI1_NSS_PIN; 
#else // remapped
 static constexpr  uint32_t            TFT_DC              = PA8; // next to SPI2 NSS
 static constexpr  uint32_t            TFT_CS              = BOARD_SPI1_ALT_NSS_PIN;
#endif
#else // SPI2
 static constexpr  uint32_t            TFT_DC              = PA8; // next to SPI2 NSS
 static constexpr  uint32_t            TFT_CS              = BOARD_SPI2_NSS_PIN;
#endif
 
#if DAC_SPI == SPI1
#ifndef REMAP_SPI1
 static constexpr  uint32_t            DAC_WS              = BOARD_SPI1_NSS_PIN;
#else // remapped
 static constexpr  uint32_t            DAC_WS              = BOARD_SPI1_ALT_NSS_PIN;
#endif
#else // SPI2
 static constexpr  uint32_t            DAC_WS              = BOARD_SPI2_NSS_PIN;
#endif

#ifndef DISABLE_SD
// #if SD_SPI == SPI1
// #ifndef REMAP_SPI1
//  static constexpr  uint32_t            SD_CS               = BOARD_SPI1_NSS_PIN;
// #else // remapped
//  static constexpr  uint32_t            SD_CS               = BOARD_SPI1_ALT_NSS_PIN;
// #endif
// #else // SPI2
//  static constexpr  uint32_t            SD_CS               = BOARD_SPI2_NSS_PIN;
// #endif
 static constexpr  uint32_t            SD_CS                = PB1;
#endif
 
private:
 static            SPIClass             _spi_1;
 static            SPIClass             _spi_2;
 static            int32_t              _avg_sample;
 static            size_t               _sample_ix0;
 static            HardwareTimer        _timer_1; 
 static            HardwareTimer        _timer_2;
 static            HardwareTimer        _timer_3;

#ifndef DISABLE_DAC
 static            dac                  _dac;
#endif
 
 static            tft                  _tft;
 static            draw_buffer          _draw_buffer;

 template <typename value_t_>
 class displayed_value {

 public:
  typedef value_t_ value_t;
 
 private:
  value_t       _value;
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
   
   if (! _drawn) {
    application::_tft.setCursor(_x_pos, _y_pos);
    application::_tft.setTextColor(_color);
    application::_tft.setTextSize(2);
    
    application::_tft.print(_name);

    _drawn = true;
   }
   else if (_value == newval) {
    return;
   }
   else {
    _value   = newval;
   }

   uint16_t x_pos = _x_pos + 11 * _name_len;
   
   application::_tft.setCursor(x_pos, _y_pos);
   
   // uint16_t font_red   = _value < 24 ? 0 : min(31, _value - 24);
   // uint16_t font_green = _value < 24 ? 0 : min(63, (_value - 24) << 1);
   uint16_t font_red = 0, font_green = 0;
   if (_value >= 56) {
    font_red   = (_value - 56) << 2;
    font_green = (_value - 56) << 3;
   }
//    else if (_value >= 32) {
// //    font_red   = (_value - 48) << 1;
//     font_green = (_value - 32) << 1;
//    }
   else if (_value <= 8) {
    font_red   = (8 - _value) << 2;
    font_green = (8 - _value) << 3;
   }
   
   uint16_t font_color = (font_red << 11) | (font_green << 5);
   
   application::_tft.setTextColor(font_color);
   application::_tft.setTextSize(2);

   uint16_t red_value   = _value >> 1;
   uint16_t green_value = 63 - _value;
   uint16_t color = (red_value << 11) | (green_value << 5);
   
   application::_tft.fillRect (
    x_pos, _y_pos - 2,
    _value, 18,
    color
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
 static            void                 setup_sd();
 static            void                 setup_spis();
 static            bool                 tenth_second();
 static            bool                 half_second();
 static            bool                 one_second();
 static            bool                 idle();
 
public:
 static            void                 setup();
 static            void                 loop();
};

#endif

/* Local Variables:  */
/* fill-column: 100  */
/* End:              */
