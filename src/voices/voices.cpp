#include "voices/voices.h"

uint32_t          voices::phincrs[120]    = { 0                         };
voices::voice *   voices::items             [ voices::COUNT             ];
uint12_t          voices::scaled_volume     { 1500                      };
uint12_t          voices::raw_volume        { 4091                      };


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

