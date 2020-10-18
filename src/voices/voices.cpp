#include "voices/voices.h"

uint32_t          voices::phincrs[120]    = { 0                         };
voices::voice *   voices::items             [ voices::COUNT             ];
uint12_t          voices::scaled_volume     { 1500                      };
uint12_t          voices::raw_volume        { 4091                      };
const uint32_t    voices::S_RATE            { 44100                     };

//////////////////////////////////////////////////////////////////////////////

bool voices::set_volume(uint12_t const & volume) {
  if (volume == raw_volume) return false;
  
  raw_volume    = volume;
  
  scaled_volume = ((raw_volume << 2) - raw_volume) >> 2;

  return true;
}

////////////////////////////////////////y//////////////////////////////////////

bool voices::set_pitch(uint8_t const & voice_ix, uint12_t const & parameter) {
  const uint8_t control_shift = 9;
  uint8_t       notes_ix      = parameter >> control_shift;
  
  static int8_t notes[(0xfff >> control_shift) + 1] = {
     0, 2, 3, 5, 7, 8, 10, 12
  };

  voices::items[voice_ix]->next_phincr =
    voices::phincrs[notes[notes_ix] + BASS_ROOT_NOTE];
  
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

