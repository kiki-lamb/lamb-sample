#include "application/application.h"
#include <inttypes.h>
#include <Arduino.h>
#include <math.h>

using namespace lamb;
using namespace lamb::Tables;

//////////////////////////////////////////////////////////////////////////////

const uint32_t               application::CAPTURE_RATIO      { 4                         };
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

void application::setup_voices() {
  voices::lpf.set_f(255);
  voices::lpf.set_q(0);
  
  voices::generate_phincrs();

  for (size_t ix = 0; ix < voices::COUNT; ix ++) {
    voices::items[ix] = new voices::voice(
      Samples::data+voices::BLOCK_SIZE*voices::MAP[ix],
      voices::BLOCK_SIZE
    );

    Serial.print(F("Voice #"));
    Serial.print(ix);
    Serial.print(F(" @ 0x "));
    Serial.print((uint32_t)&voices::items[ix]);
    Serial.print(F(" => 0x"));
    Serial.print(((uint32_t)Samples::data+voices::BLOCK_SIZE*voices::MAP[ix]), HEX);
    Serial.println();
    
    voices::items[ix]->phincr    = voices::phincrs[voices::ROOT_NOTE];
    voices::items[ix]->amplitude = 0x80;
  }

   voices::items[0]->amplitude = 0xf0; // 0xb8; // kick
   voices::items[1]->amplitude = 0x40; // 0xd8; // snare
   voices::items[2]->amplitude = 0x80; // 0xd8; // oh
   voices::items[3]->amplitude = 0xe0; // 0x78; // bass
   voices::items[4]->amplitude = 0xe0; // bass
   voices::items[5]->amplitude = 0xe0; // bass

   voices::items[3]->phincr = voices::phincrs[voices::BASS_ROOT_NOTE +  0   ];
   voices::items[4]->phincr = voices::phincrs[voices::BASS_ROOT_NOTE +  0   ];
   voices::items[5]->phincr = voices::phincrs[voices::BASS_ROOT_NOTE - 12   ];
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
  maple_timer::setup(_timer_1, voices::S_RATE, s_rate);
  maple_timer::setup(_timer_2, K_RATE, k_rate);
}
////////////////////////////////////////////////////////////////////////////////

bool application::graph() {
  if (! _draw_buffer.readable()) return false;
  
  static       uint16_t col = 0;
  static const uint16_t col_max = 200; // real max 320
  uint16_t              tmp_col = col+120;
  
  _tft.drawFastVLine(tmp_col, 0, 240, ILI9341_BLACK);

  uint16_t              tmp_volume = 119 - map(voices::raw_volume, 0, 4091, 0, 119);

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
  static size_t  buttons[]      = {  PB11 ,   PB10 ,    PB1 ,   PB0,   PC14 ,  PC15   };
  static char *  button_names[] = { "PB11",  "PB10",   "PB1",  "PB0", "PC14", "PC15"  };

  static uint8_t last_trigger_states = 0;
  uint8_t        trigger_states      = 0;

  ::controls::poll();

  while(application_event ae = ::controls::dequeue_event())
  {
    switch (ae.type) {
    case application_event_type::EVT_VOLUME:
    {
      voices::set_volume(ae.parameter);

      break;
    }
    case application_event_type::EVT_TRIGGER:
    {
      trigger_states |= 1 << ae.parameter;
      
      break;
    }
    case application_event_type::EVT_PITCH_1:
    {
      voices::set_pitch(3, ae.parameter);
      
      break;     
    }
    case application_event_type::EVT_PITCH_2:
    {
      voices::set_pitch(4, ae.parameter);
      
      break;     
    }
    case application_event_type::EVT_PITCH_3:
    {
      voices::set_pitch(5, ae.parameter);
      
      break;     
    }
    case application_event_type::EVT_FILTER_F_1:
    {
      voices::lpf.set_f(ae.parameter);
      
      break;     
    }
    case application_event_type::EVT_FILTER_Q_1:
    {
      if      (voices::lpf.f > 80) 
        voices::lpf.set_q(min(250, ae.parameter));
      else if (voices::lpf.f > 60) 
        voices::lpf.set_q(min(240, ae.parameter));
      else if (voices::lpf.f > 40) 
        voices::lpf.set_q(min(230, ae.parameter));
      else 
        voices::lpf.set_q(min(220, ae.parameter));
      
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
  
  for (size_t ix = 0; ix < voices::COUNT; ix ++) {
    if (
      (trigger_states & (1 << ix)) &&
      (! (last_trigger_states & (1 << ix)))
    ) {
      voices::items[ix]->trigger();

      if (ix >= 3) {
        voices::items[3]->stop();
        voices::items[4]->stop();
        voices::items[5]->stop();
      }
      
    }
  }

  last_trigger_states = trigger_states;
}

////////////////////////////////////////////////////////////////////////////////

void application::s_rate() {
  if (_sample_ix0 == (1 << CAPTURE_RATIO)) {
    _avg_sample >>= CAPTURE_RATIO;

    if (_draw_buffer.writable()) {
      _draw_buffer.enqueue(_avg_sample); // 
    }

    _sample_ix0 = 0;
    _avg_sample = 0;
  }

  sample_type_traits<voices::sample>::mix_type drums_ =
    sample_type_traits<sample_type_traits<voices::sample>::mix_type>::silence;

  sample_type_traits<voices::sample>::mix_type bass_ =
    sample_type_traits<sample_type_traits<voices::sample>::mix_type>::silence;

  auto v = voices::items;
  v += 3;

  MIX(drums_, voices::items,     3);

  MIX(bass_,  v, 3);

  bass_ >>= 2;
  
  bass_ = voices::lpf.process(bass_);

  drums_ >>= 2;
  
  drums_ += bass_;
  
  AMPLIFY(drums_, voices::scaled_volume, 9);
  
  _avg_sample += drums_;

  _dac.write_mono(drums_);

  _sample_ix0  ++;
  _sample_ix1  ++;
}

////////////////////////////////////////////////////////////////////////////////

void application::setup() {
  delay(3000);
  
  Serial.begin(115200);
  
  setup_voices();
  ::controls::setup();
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
    static const uint8_t avging                    = 8;      
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
    Serial.print(F(", "));
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
