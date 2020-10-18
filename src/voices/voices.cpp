#include "voices/voices.h"

////////////////////////////////////////////////////////////////////////////////

const uint32_t       voices::S_RATE            { 44100                     };
voices::voice *      voices::_items            [ voices::COUNT             ];
uint32_t             voices::_phincrs[120]   = { 0                         };
uint12_t             voices::_volume           { 2000                      };
uint12_t             voices::_scaled_volume    { 1500                      };
lamb::lowpass_filter voices::_lpf;

////////////////////////////////////////////////////////////////////////////////

void voices::trigger(uint8_t const & ix) {
    item(ix).trigger();

#ifdef LOG_TRIGGERS
    Serial.print("Trigger ");
    Serial.print(ix);
    Serial.println();
#endif
    
    if (ix >= 3) {
      voices::item(3).stop();
      voices::item(4).stop();
      voices::item(5).stop();
    }
}

////////////////////////////////////////////////////////////////////////////////

voices::voice & voices::item(size_t const & ix) {
  return *(_items[ix]);
}

////////////////////////////////////////////////////////////////////////////////

uint12_t voices::volume() {
  return _volume;
}
////////////////////////////////////////////////////////////////////////////////

uint12_t voices::scaled_volume() {
  return _scaled_volume;
}

////////////////////////////////////////////////////////////////////////////////

uint8_t voices::filter_f() {
  return _lpf.f;
}
  
////////////////////////////////////////////////////////////////////////////////

uint8_t voices::filter_q() {
  return _lpf.q;
}

////////////////////////////////////////////////////////////////////////////////

void voices::filter_f(uint8_t const & f_) {
  _lpf.f = f_;
}
  
////////////////////////////////////////////////////////////////////////////////

void voices::filter_q(uint8_t const & q_) {
  _lpf.q = q_;
}
  
////////////////////////////////////////////////////////////////////////////////

void voices::setup() {
  _lpf.set_f(255);
  _lpf.set_q(0);
  
  generate_phincrs();

  for (size_t ix = 0; ix < COUNT; ix ++) {
    _items[ix] = new voice(
      Samples::data+BLOCK_SIZE*MAP[ix],
      BLOCK_SIZE
    );

    Serial.print(F("Voice #"));
    Serial.print(ix);
    Serial.print(F(" @ 0x "));
    Serial.print((uint32_t)&_items[ix]);
    Serial.print(F(" => 0x"));
    Serial.print(((uint32_t)Samples::data+BLOCK_SIZE*MAP[ix]), HEX);
    Serial.println();
    
    _items[ix]->phincr    = _phincrs[ROOT_NOTE];
    _items[ix]->amplitude = 0x80;
  }

   _items[0]->amplitude = 0xf0; // 0xb8; // kick
   _items[1]->amplitude = 0x40; // 0xd8; // snare
   _items[2]->amplitude = 0x80; // 0xd8; // oh
   _items[3]->amplitude = 0xe0; // 0x78; // bass
   _items[4]->amplitude = 0xe0; // bass
   _items[5]->amplitude = 0xe0; // bass

   _items[3]->phincr = _phincrs[BASS_ROOT_NOTE +  0   ];
   _items[4]->phincr = _phincrs[BASS_ROOT_NOTE +  0   ];
   _items[5]->phincr = _phincrs[BASS_ROOT_NOTE - 12   ];
}

//////////////////////////////////////////////////////////////////////////////

voices::sample voices::read() {
  mix mixed = SILENCE;
  mix bass  = SILENCE;
  auto v    = _items;

  v        += 3;

  MIX(mixed, _items, 3);
  MIX(bass , v,      3);

  bass    >>= 2;  
  bass      = _lpf.process(bass );
  mixed   >>= 2;  
  mixed    += bass ;
  
  AMPLIFY(mixed, _scaled_volume, 9);

  return mixed;
}

//////////////////////////////////////////////////////////////////////////////

bool voices::volume(uint12_t const & volume) {
  if (volume == _volume) return false;
  
  _volume    = volume;
  
  return true;
}

///////////////////////////////////////////////////////////////////////////////

bool voices::pitch(uint8_t const & voice_ix, uint12_t const & parameter) {
  const uint8_t control_shift = 9;
  uint8_t       notes_ix      = parameter >> control_shift;
  
  static int8_t notes[(0xfff >> control_shift) + 1] = {
     0, 2, 3, 5, 7, 8, 10, 12
  };

  _items[voice_ix]->next_phincr =
    _phincrs[notes[notes_ix] + BASS_ROOT_NOTE];
  
  return true;
}

////////////////////////////////////////////////////////////////////////////////

void voices::generate_phincrs() {
  Serial.print(F("\n\nGenerating..."));
  Serial.println();
  
  auto start = millis();

  static const uint32_t S_RATE_2 = S_RATE << 1;

  uint32_t one_hz = lamb::Tables::generate_phase_increment(S_RATE_2, 1);
  Serial.print(F("1 hz = "));
  Serial.print(one_hz);
  Serial.println();

  for (int8_t octave = -MIDDLE_OCTAVE; octave < (10 - MIDDLE_OCTAVE); octave ++) {
    for (size_t note = 0; note < 12; note ++) {
      size_t write_ix = (octave + MIDDLE_OCTAVE) * 12 + note;
      
      Serial.print(F("ix "));
      Serial.print(write_ix);
      Serial.print(F(" octave "));
      Serial.print(octave);
      Serial.print(F(" note "));
      
      if (note < 10) {
        Serial.print(' ');
      }
      
      Serial.print(note);
      Serial.print(' ');
      
      uint32_t tmp_phincr = lamb::Tables::generate_phase_increment(
        S_RATE_2,
        lamb::midi_notes::twelve_tet_data[note]
      );
      
      if (octave < 0) {
         tmp_phincr >>= (octave * -1);
      }
      else {
         tmp_phincr <<= octave;
      }
      
      Serial.print(tmp_phincr);
      
      if (tmp_phincr == one_hz) {
        Serial.print(F("                <== 1 hz"));
      }

      Serial.println();

      voices::_phincrs[write_ix] = tmp_phincr;
    }
  }
 
  Serial.print(F("1 hz = "));
  Serial.print(one_hz);
  Serial.println();
  
  Serial.print(F("Done after "));
  Serial.print(millis() - start);
  Serial.print(F(" ms."));
  Serial.println();
}

////////////////////////////////////////////////////////////////////////////////
