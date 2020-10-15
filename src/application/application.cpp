#include "application/application.h"
#include <inttypes.h>
#include <Arduino.h>
#include <math.h>

using namespace lamb;
using namespace lamb::Tables;

//////////////////////////////////////////////////////////////////////////////

const uint8_t                application::NOTE               { 24                        };
const uint8_t                application::BASS_ROOT_NOTE     { application::NOTE - 6     };
const uint32_t               application::K_RATE             { 100                       };
const uint32_t               application::S_RATE             { 88200                     };
uint32_t                     application::_phincrs[120]   =  { 0                         };
int32_t                      application::_avg_sample        { 0                         };
uint12_t                     application::_scaled_volume     { 2048                      };
uint12_t                     application::_raw_volume        { 4091                      };
uint16_t                     application::_knob1             { 4091                      };
uint16_t                     application::_knob2             { 4091                      };
size_t                       application::_sample_ix         { 0                         };
HardwareTimer                application::_timer_1           ( 1                         );
HardwareTimer                application::_timer_2           ( 2                         );
HardwareTimer                application::_timer_3           ( 3                         );
application::voice *         application::_voices            [ 6                         ];
application::signal          application::_signal_device0    ( PA0,  8, 4                );
application::signal          application::_signal_device1    ( PA1,  8, 6                );
application::signal          application::_signal_device2    ( PA2,  8, 6                );
application::signal_source   application::_signal_source0    ( &_signal_device0          );
application::signal_source   application::_signal_source1    ( &_signal_device1          );
application::signal_source   application::_signal_source2    ( &_signal_device2          );
application::button          application::_button_device0    ( PB11, 0                   );
application::button          application::_button_device1    ( PB10, 1                   );
application::button          application::_button_device2    ( PB1,  2                   );
application::button          application::_button_device3    ( PB0,  3                   );
application::button          application::_button_device4    ( PC15, 4                   );
application::button          application::_button_device5    ( PC14, 5                   );
application::button_source   application::_button_source0    ( &_button_device0          );
application::button_source   application::_button_source1    ( &_button_device1          );
application::button_source   application::_button_source2    ( &_button_device2          );
application::button_source   application::_button_source3    ( &_button_device3          );
application::button_source   application::_button_source4    ( &_button_device4          );
application::button_source   application::_button_source5    ( &_button_device5          );
application::dac             application::_dac               ( application::I2S_WS, &SPI );
application::tft             application::_tft(application::TFT_CS, application::TFT_DC  );
application::draw_buffer     application::_draw_buffer;         
application::combined_source application::_combined_source;
application::control_source  application::_control_event_source;

//////////////////////////////////////////////////////////////////////////////

void application::setup_controls() {
  _signal_device0      .setup();
  _signal_device1      .setup();
  _signal_device2      .setup();
      
  _button_device0      .setup();
  _button_device1      .setup();
  _button_device2      .setup();
  _button_device3      .setup();
  _button_device4      .setup();
  _button_device5      .setup();

  _combined_source     .sources[0]  = &_signal_source0;
  _combined_source     .sources[1]  = &_signal_source1;
  _combined_source     .sources[2]  = &_signal_source2;

  _combined_source     .sources[3]  = &_button_source0;
  _combined_source     .sources[4]  = &_button_source1;
  _combined_source     .sources[5]  = &_button_source2;
  _combined_source     .sources[6]  = &_button_source3;
  _combined_source     .sources[7]  = &_button_source4;
  _combined_source     .sources[8]  = &_button_source5;

  _control_event_source.source      = &_combined_source;
}

////////////////////////////////////////////////////////////////////////////////

void application::generate_phincrs() {
  Serial.print("\n\nGenerating...");
  Serial.println();
  
  auto start = millis();
  
  uint32_t one_hz = generate_phase_increment(S_RATE, 1);
  Serial.print("1 hz = ");
  Serial.print(one_hz);
  Serial.println();
  
  for (int8_t octave = -3; octave < 7; octave++) {
    for (size_t note = 0; note < 12; note++) {
      size_t write_ix = (octave + 3) * 12 + note;
      
      Serial.print("ix ");
      Serial.print(write_ix);
      Serial.print(" octave ");
      Serial.print(octave);
      Serial.print(" note ");
      
      if (note < 10) {
        Serial.print(" ");
      }
      
      Serial.print(note);
      Serial.print(" ");
      
      uint32_t tmp_phincr = generate_phase_increment(
        S_RATE,
        midi_notes::twelve_tet_data[note]
      );
      
      if (octave < 0) {
         tmp_phincr >>= (octave * -1);
      }
      else {
         tmp_phincr <<= octave;
      }
      
      Serial.print(tmp_phincr);
      
      if (tmp_phincr == one_hz) {
        Serial.print("                <== 1 hz");
      }

      Serial.println();

      _phincrs[write_ix] = tmp_phincr;
    }
  }
 
  Serial.print("1 hz = ");
  Serial.print(one_hz);
  Serial.println();
  
  Serial.print("Done after ");
  Serial.print(millis() - start);
  Serial.print(" ms.");
  Serial.println();
}

////////////////////////////////////////////////////////////////////////////////

void application::setup_voices() {
  generate_phincrs();

  for (size_t ix = 0; ix < 6; ix++) {
    _voices[ix] = new voice(
      Samples::data+BLOCK_SIZE*_voices_map[ix],
      BLOCK_SIZE
    );

    Serial.print("Voice #");
    Serial.print(ix);
    Serial.print(" @ 0x ");
    Serial.print((uint32_t)&_voices[ix]);
    Serial.print(" => 0x");
    Serial.print(((uint32_t)Samples::data+BLOCK_SIZE*_voices_map[ix]), HEX);
    Serial.println();
    
    _voices[ix]->phincr    = _phincrs[NOTE];
    _voices[ix]->amplitude = 0x80;
  }

  // _voices[0]->amplitude = 0xd0; // 0xb8; // kick
  // _voices[1]->amplitude = 0xf0; // 0xd8; // hi bass
  // _voices[2]->amplitude = 0xf0; // 0xd8; // lo bass
  // _voices[3]->amplitude = 0x48; // 0x78; // snare 
  // _voices[4]->amplitude = 0xff; // closed hat
  // _voices[5]->amplitude = 0xa0; // open hat
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
  maple_timer::setup(_timer_1, S_RATE, s_rate);
  maple_timer::setup(_timer_2, K_RATE, k_rate);
}

////////////////////////////////////////////////////////////////////////////////

application::application_event application::process_control_event(
  application::control_event const & control_event
) {
  application_event application_event;
  application_event.type = application_event_type::EVT_UNKNOWN;

  switch (control_event.type) {
  case control_event_type::CTL_EVT_NOT_AVAILABLE:
    application_event.type = application_event_type::APP_EVT_NOT_AVAILABLE;    
    return application_event;
    
  case control_event_type::EVT_SIGNAL:
    return process_signal_event(control_event);

  case control_event_type::EVT_BUTTON:
    return process_button_event(control_event);  
  }
  
  return application_event;
}

////////////////////////////////////////////////////////////////////////////////

application::application_event application::process_signal_event(
  application::control_event const & control_event
) {
  uint16_t sig_val = control_event.parameter & 0x0fff;
  uint8_t  sig_num = (control_event.parameter& 0xf000) >> 12;

  application_event application_event;

  if (sig_num == 2) {
    application_event.type           = application_event_type::EVT_VOLUME;
    application_event.parameter      = sig_val;
  }
  else if (sig_num == 0) {
    application_event.type           = application_event_type::EVT_PITCH_1;
    application_event.parameter      = sig_val;
  }
  else if (sig_num == 1) {
    application_event.type           = application_event_type::EVT_PITCH_2;
    application_event.parameter      = sig_val;
  }
  else {
    application_event.type           = application_event_type::APP_EVT_NOT_AVAILABLE;
  }

  // Serial.print("Signal ");
  // Serial.print(sig_num);
  // Serial.print(" = ");
  // Serial.print (sig_val);
  // Serial.println();

  return application_event;
}

////////////////////////////////////////////////////////////////////////////////

application::application_event application::process_encoder_event(
  application::control_event const & control_event
) {
  application_event application_event;
  application_event.type           = application_event_type::EVT_UNKNOWN;

  Serial.println("Don't know how to process encoders yet!");

  return application_event;
}

////////////////////////////////////////////////////////////////////////////////

application::application_event application::process_button_event(
  application::control_event const & control_event
) {
  application_event application_event;
  application_event.type           = application_event_type::EVT_UNKNOWN;
  uint8_t           button_number  = control_event.parameter_hi();
  int8_t            button_state   = (int8_t)control_event.parameter_lo(); 
  
  // Serial.print(F("Button event, number: "));
  // Serial.print(button_number);
  // Serial.print(F(", state: "));
  // Serial.print(button_state);
  // Serial.println();

  application_event.type      = application_event_type::EVT_TRIGGER;
  application_event.parameter = button_number;
  
  return application_event;
}

////////////////////////////////////////////////////////////////////////////////

void application::graph() {
  static      uint16_t  col = 0;
  static const uint16_t col_max = 200; // real max 320
  uint16_t              tmp_col = col+120;
  
  _tft.drawFastVLine(tmp_col, 0, 240, ILI9341_BLACK);

  uint16_t              tmp_volume = 119 - map(_raw_volume, 0, 4091, 0, 119);

  _tft.drawPixel(tmp_col, tmp_volume, ILI9341_GREEN);
  _tft.drawPixel(tmp_col, 239-tmp_volume, ILI9341_GREEN);

  sample                tmp = _draw_buffer.dequeue();

  sample                tmp_sample = map(tmp, -32768, 32767, -120, 119);
  
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
}

//////////////////////////////////////////////////////////////////////////////

bool application::volume(uint12_t const & volume) {
  if (volume == _raw_volume) return false;
  
  _raw_volume    = volume;
  
  _scaled_volume = ((_raw_volume << 2) - _raw_volume) >> 2;

  return true;
}

////////////////////////////////////////y//////////////////////////////////////

bool application::pitch(uint8_t const & voice_ix, uint12_t const & parameter) {
  const uint8_t control_shift = 9;
  uint8_t       notes_ix      = parameter >> control_shift;
  
  static int8_t notes[(0xfff >> control_shift) + 1] = {
     0, 2, 3, 5, 7, 9, 11, 12
  };

   if (voice_ix == 2) {
     uint16_t tmp_parameter = parameter;
     tmp_parameter += 15;
     
     // Serial.print(tmp_parameter);
     
     uint8_t lin = 0;
     uint12_t mask = 0x800;

     for(uint8_t ix = 0; ix < 8; ix++) {
       if (mask & tmp_parameter) {
         lin += 1;
       }
       mask >>= 1;
     }

     // Serial.print(" => ");
     // Serial.print(lin);
     // Serial.println();
   
     notes_ix = lin;
   }
   
   // Serial.print(voice_ix);
   // Serial.print(" = ");
   // Serial.print(notes_ix);
   // Serial.print(" ");
   // Serial.print(notes[notes_ix] + BASS_ROOT_NOTE);
   // Serial.print(" ");
   // Serial.print(_phincrs[notes[notes_ix] + BASS_ROOT_NOTE]);
   // Serial.println();
 
   _voices[voice_ix]->next_phincr =
     _phincrs[notes[notes_ix] + BASS_ROOT_NOTE];

  return true;
}

//////////////////////////////////////////////////////////////////////////////

void application::k_rate() {
  static size_t  buttons[]      = {  PB11 ,   PB10 ,    PB1 ,   PB0,   PC14 ,  PC15   };
  static char *  button_names[] = { "PB11",  "PB10",   "PB1",  "PB0", "PC14", "PC15"  };

  static uint8_t last_trigger_states = 0;
  uint8_t        trigger_states      = 0;

  _control_event_source.poll();

  while(
    application_event ae = process_control_event(
      _control_event_source.dequeue_event()
    )
  ) {
    switch (ae.type) {
    case application_event_type::EVT_VOLUME:
    {
      volume(ae.parameter);

      break;
    }
    case application_event_type::EVT_TRIGGER:
    {
      trigger_states |= 1 << ae.parameter;
      
      break;
    }
    case application_event_type::EVT_PITCH_1:
    {
      // pitch(1, ae.parameter);
      
      break;     
    }
    case application_event_type::EVT_PITCH_2:
    {
      // pitch(2, ae.parameter);
      
      break;     
    }
    default:
    {
      Serial.print("Unrecognized event: ");
      Serial.print(ae.type);
      Serial.println();
    }
    }
  }
  
  for (size_t ix = 0; ix < 6; ix++) {
    if (
      (trigger_states & (1 << ix)) &&
      (! (last_trigger_states & (1 << ix)))
    ) {
      _voices[ix]->trigger  = true;
    }
  }

  last_trigger_states = trigger_states;
}


////////////////////////////////////////////////////////////////////////////////
  
void application::s_rate() {
  if ((_sample_ix % (1 << CAPTURE_RATIO)) == 0) {
    _avg_sample >>= CAPTURE_RATIO;

    if (_draw_buffer.writable()) {
      _draw_buffer.enqueue(_avg_sample); // 
    }
    
    _avg_sample = 0;
  }
  
  int32_t sample_ = 0;
  
  for (size_t ix = 0; ix < 6; ix++) {
    sample_ += _voices[ix]->play();
    // Serial.print(ix);
    // Serial.print(" ");
    // Serial.println(_voices[ix]->phincr);
  }

  sample_     *= _scaled_volume;
  sample_    >>= 12;
  
  _avg_sample += sample_;

  _dac.write_mono(sample_);
  _sample_ix  ++;
}

////////////////////////////////////////////////////////////////////////////////

void application::setup() {
  delay(3000);
  
  Serial.begin(115200);
  
  setup_voices();
  setup_controls();
  setup_tft();
  setup_dac();
  setup_timers();
}
  
////////////////////////////////////////////////////////////////////////////////

void application::loop(void) {
  if (_draw_buffer.readable()) {
    graph();
  }
}

////////////////////////////////////////////////////////////////////////////////

/* Local Variables:  */
/* fill-column: 100  */
/* End:              */
