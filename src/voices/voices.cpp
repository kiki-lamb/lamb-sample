#include "voices/voices.h"

const uint32_t       voices::S_RATE            { 44100                     };
uint32_t             voices::phincrs[120]    = { 0                         };
voices::voice *      voices::items             [ voices::COUNT             ];
uint12_t             voices::_scaled_volume    { 1500                      };
uint12_t             voices::_raw_volume       { 4091                      };
lamb::lowpass_filter voices::lpf;

////////////////////////////////////////////////////////////////////////////////

uint12_t voices::raw_volume() {
  return _raw_volume;
}
////////////////////////////////////////////////////////////////////////////////

uint12_t voices::scaled_volume() {
  return _scaled_volume;
}

////////////////////////////////////////////////////////////////////////////////

uint8_t voices::filter_f() {
  return lpf.f;
}
  
////////////////////////////////////////////////////////////////////////////////

uint8_t voices::filter_q() {
  return lpf.q;
}

////////////////////////////////////////////////////////////////////////////////

void voices::filter_f(uint8_t const & f_) {
  lpf.f = f_;
}
  
////////////////////////////////////////////////////////////////////////////////

void voices::filter_q(uint8_t const & q_) {
  lpf.q = q_;
}
  
////////////////////////////////////////////////////////////////////////////////

void voices::setup() {
  lpf.set_f(255);
  lpf.set_q(0);
  
  generate_phincrs();

  for (size_t ix = 0; ix < COUNT; ix ++) {
    items[ix] = new voice(
      Samples::data+BLOCK_SIZE*MAP[ix],
      BLOCK_SIZE
    );

    Serial.print(F("Voice #"));
    Serial.print(ix);
    Serial.print(F(" @ 0x "));
    Serial.print((uint32_t)&items[ix]);
    Serial.print(F(" => 0x"));
    Serial.print(((uint32_t)Samples::data+BLOCK_SIZE*MAP[ix]), HEX);
    Serial.println();
    
    items[ix]->phincr    = phincrs[ROOT_NOTE];
    items[ix]->amplitude = 0x80;
  }

   items[0]->amplitude = 0xf0; // 0xb8; // kick
   items[1]->amplitude = 0x40; // 0xd8; // snare
   items[2]->amplitude = 0x80; // 0xd8; // oh
   items[3]->amplitude = 0xe0; // 0x78; // bass
   items[4]->amplitude = 0xe0; // bass
   items[5]->amplitude = 0xe0; // bass

   items[3]->phincr = phincrs[BASS_ROOT_NOTE +  0   ];
   items[4]->phincr = phincrs[BASS_ROOT_NOTE +  0   ];
   items[5]->phincr = phincrs[BASS_ROOT_NOTE - 12   ];
}

//////////////////////////////////////////////////////////////////////////////

voices::sample voices::read() {
  mix_type mixed = silence;
  mix_type bass  = silence;

  auto v  = items;
  v      += 3;

  MIX(mixed, items, 3);
  MIX(bass , v,     3);

  bass  >>= 2;  
  bass    = lpf.process(bass );
  mixed >>= 2;  
  mixed  += bass ;
  
  AMPLIFY(mixed, _scaled_volume, 9);

  return mixed;
}

//////////////////////////////////////////////////////////////////////////////

bool voices::volume(uint12_t const & volume) {
  if (volume == _raw_volume) return false;
  
  _raw_volume    = volume;
  
  _scaled_volume = ((_raw_volume << 2) - _raw_volume) >> 2;

  return true;
}

///////////////////////////////////////////////////////////////////////////////

bool voices::pitch(uint8_t const & voice_ix, uint12_t const & parameter) {
  const uint8_t control_shift = 9;
  uint8_t       notes_ix      = parameter >> control_shift;
  
  static int8_t notes[(0xfff >> control_shift) + 1] = {
     0, 2, 3, 5, 7, 8, 10, 12
  };

  items[voice_ix]->next_phincr =
    phincrs[notes[notes_ix] + BASS_ROOT_NOTE];
  
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

      voices::phincrs[write_ix] = tmp_phincr;
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

