#include "application/application.h"
#include <inttypes.h>
#include <Arduino.h>
#include <math.h>

using namespace lamb;

using namespace lamb::tables;

//////////////////////////////////////////////////////////////////////////////

const uint32_t               application::CAPTURE_RATIO      { 3                         };
const uint32_t               application::V_SPACING          { 48                        };
const uint32_t               application::K_RATE             { 80                        };
int32_t                      application::_avg_sample        { 0                         };
size_t                       application::_sample_ix0        { 0                         };
size_t                       application::_sample_ix1        { 0                         };
HardwareTimer                application::_timer_1           ( 1                         );
HardwareTimer                application::_timer_2           ( 2                         );
HardwareTimer                application::_timer_3           ( 3                         );
application::dac             application::_dac               ( application::I2S_WS, &SPI );
application::tft             application::_tft(application::TFT_CS, application::TFT_DC  );
application::draw_buffer     application::_draw_buffer;         

////////////////////////////////////////////////////////////////////////////////

bool application::graph() {
 if (! _draw_buffer.readable()) return false;
  
 static       uint16_t col = 0;
 static const uint16_t col_max = 200; // real max 320
 uint16_t              tmp_col = col+120;
  
 _tft.drawFastVLine(tmp_col, 0, 240, ILI9341_BLACK);

 uint16_t              tmp_volume = 119 - map(
  voices::volume(),
  0, 4091, 0, 119
 );

 _tft.drawPixel(tmp_col, tmp_volume, ILI9341_GREEN);
 _tft.drawPixel(tmp_col, 239-tmp_volume, ILI9341_GREEN);

 voices::sample        tmp = _draw_buffer.dequeue();

 voices::sample        tmp_sample = map(tmp, -32768, 32767, -120, 119);
  
 if (tmp_sample > 0)
  _tft.drawFastVLine(
   tmp_col,
   120,
   tmp_sample,
   ILI9341_YELLOW
  );
 else 
  if (tmp_sample < 0) {
   _tft.drawFastVLine(
    tmp_col,
    120 + tmp_sample,
    abs(tmp_sample),
    ILI9341_YELLOW);
  }
  
 col ++;
 col %= col_max;
  
 return true;
}

//////////////////////////////////////////////////////////////////////////////

void application::k_rate() {
 ::controls::poll();

 while(application_event ae = ::controls::dequeue_event())
 {
  switch (ae.type) {
  case application_event_type::EVT_VOLUME:
  {
   voices::volume(ae.parameter);

   break;
  }
  case application_event_type::EVT_TRIGGER:
  {
   voices::trigger(ae.parameter);
      
   break;
  }
  case application_event_type::EVT_PITCH_1:
  {
   voices::pitch(3, ae.parameter);
      
   break;     
  }
  case application_event_type::EVT_PITCH_2:
  {
   voices::pitch(4, ae.parameter);
      
   break;     
  }
  case application_event_type::EVT_PITCH_3:
  {
   voices::pitch(5, ae.parameter);
      
   break;     
  }
  case application_event_type::EVT_FILTER_F_1:
  {
   lamb::u0q16 parameter(ae.parameter << 4); // 12 sig bits to 16

   // Serial.print("F: ");
   // Serial.println(parameter.value);
   
   voices::filter_f(parameter);
     
   break;     
  }
  case application_event_type::EVT_FILTER_Q_1:
  {
   lamb::u0q16 parameter(ae.parameter << 4); // 12 sig bits to 16
     
   voices::filter_q(parameter);
     
   break;     
  }
  default:
  {
   Serial.print(F("Unrecognized event: "));
   Serial.print(ae.type);
   Serial.println();
  }
  }
 }
}

////////////////////////////////////////////////////////////////////////////////

void application::s_rate() {
 if (_sample_ix0 == (1 << CAPTURE_RATIO)) {
  _avg_sample >>= CAPTURE_RATIO;

  if (_draw_buffer.writable()) {
   _draw_buffer.enqueue(_avg_sample);
  }

  _sample_ix0 = 0;
  _avg_sample = 0;
 }

 voices::mix s = voices::read();

 _avg_sample  += s;

 _dac.write_mono(s);

 _sample_ix0  ++;
 _sample_ix1  ++;
}
 
////////////////////////////////////////////////////////////////////////////////
      
void application::setup_tft() {
 _tft.begin();
 _tft.setRotation(3);
 _tft.setTextColor(ILI9341_WHITE);  
 _tft.setTextSize(2);
 _tft.fillScreen(ILI9341_BLACK);
}

////////////////////////////////////////////////////////////////////////////////


void application::setup_dac() {
 SPI.begin();
  
 _dac.setup();
}

////////////////////////////////////////////////////////////////////////////////

void application::setup_timers() {
 device::maple_timer::setup(_timer_1, voices::S_RATE, s_rate);
 device::maple_timer::setup(_timer_2, K_RATE, k_rate);
}

////////////////////////////////////////////////////////////////////////////////

void application::setup() {
 delay(3000);
  
 Serial.begin(64000000);
  
 voices::setup();
 ::controls::setup();

 afio_remap(AFIO_REMAP_USART1);
 afio_cfg_debug_ports (AFIO_DEBUG_SW_ONLY);
 afio_remap (AFIO_REMAP_SPI1);
 gpio_set_mode (GPIOA, 15, GPIO_AF_OUTPUT_PP);
 gpio_set_mode (GPIOB,  3, GPIO_AF_OUTPUT_PP);
 gpio_set_mode (GPIOB,  4, GPIO_INPUT_FLOATING);
 gpio_set_mode (GPIOB,  5, GPIO_AF_OUTPUT_PP);
 
 setup_tft();
 setup_dac();
 setup_timers();
}
  
////////////////////////////////////////////////////////////////////////////////

void application::loop() {
 static uint32_t draw_operations                  = 0;
  
 if (graph())
  draw_operations ++;
  
#ifdef LOG_DRAW_RATES
 if (_sample_ix1 >= (voices::S_RATE / 10)) {
  static const uint8_t avging                    = 10;      
  static uint32_t      avg_draw_operations       = 0;
  static uint32_t      tenth_seconds             = 0;
  tenth_seconds                                 += 1;
  _sample_ix1                                    = 0;
  uint32_t             tmp_avg_draw_operations   = avg_draw_operations;
  avg_draw_operations                           *= avging;
  avg_draw_operations                           -= tmp_avg_draw_operations;
  avg_draw_operations                           += draw_operations;
  avg_draw_operations                           /= avging;    
    
  Serial.print(tenth_seconds);
  Serial.print(F(", "));
  Serial.print(draw_operations);
  Serial.print(F(", "));
  Serial.print(avg_draw_operations);
  Serial.print(F(" avg, "));
  Serial.print(
   (int32_t)avg_draw_operations -
   (int32_t)tmp_avg_draw_operations
  );
  Serial.print(F(", "));
  Serial.print(_draw_buffer.count());
  Serial.println();
    
  draw_operations                                = 0;
 }
#endif
}

////////////////////////////////////////////////////////////////////////////////

/* Local Variables:  */
/* fill-column: 100  */
/* End:              */
