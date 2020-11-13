#include "voices/voices.h"

////////////////////////////////////////////////////////////////////////////////

using namespace lamb;

////////////////////////////////////////////////////////////////////////////////

const uint32_t       voices::S_RATE            { 32768                         };
voices::voice *      voices::_items            [ voices::COUNT                 ];
u0q32                voices::_phincrs[120]   = { u0q32(0)                      };
voices::volume_type  voices::_volume           { 1000                          };
voices::filter       voices::_lpf;

////////////////////////////////////////////////////////////////////////////////

void voices::setup() {
 _lpf.freq(u17q15(1000));
 _lpf.res(u16q16(0));
  
 generate_phincrs();

 for (size_t ix = 0; ix < COUNT; ix ++) {
  _items[ix] = new voice(
   Samples::data+BLOCK_SIZE*MAP[ix],
   BLOCK_SIZE
  );

  Serial.print(F("Voice #"));
  Serial.print(ix);
  Serial.print(F(" @ 0x "));
  Serial.print((uint32_t) &item(ix));
  Serial.print(F(" => 0x"));
  Serial.print(((uint32_t)Samples::data+BLOCK_SIZE*MAP[ix]), HEX);
  Serial.println();
   
  item(ix).phincr    = u0q32(_phincrs[ROOT_NOTE]);
  item(ix).amplitude = u0q8(0x80);
 }

 item(0).amplitude = u0q8(0xd0); // 0xb8; // kick
 item(1).amplitude = u0q8(0x50); // 0xd8; // snare
 item(2).amplitude = u0q8(0x60); // 0xd8; // oh
 item(3).amplitude = u0q8(0xd0); // 0x78; // bass
 item(4).amplitude = u0q8(0xd0); // bass
 item(5).amplitude = u0q8(0xd0); // bass

 item(3).phincr = _phincrs[BASS_ROOT_NOTE +  0  ];
 item(4).phincr = _phincrs[BASS_ROOT_NOTE +  0  ];
 item(5).phincr = _phincrs[BASS_ROOT_NOTE +  1  ];
}

////////////////////////////////////////////////////////////////////////////////

void voices::generate_phincrs() {
 Serial.print(F("\n\nGenerating..."));
 Serial.println();
  
 auto start = millis();

 static const uint32_t S_RATE_2 = S_RATE << 1;

 uint32_t one_hz = tables::generate_phase_increment(S_RATE_2, 1);
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
      
   uint32_t tmp_phincr = tables::generate_phase_increment(
    S_RATE_2,
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
    Serial.print(F("                <== 1 hz"));
   }

   Serial.println();

   voices::_phincrs[write_ix] = u0q32(tmp_phincr);
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

voices::volume_type voices::volume() {
 return _volume;
}

////////////////////////////////////////////////////////////////////////////////

voices::filter::unsigned_internal_t voices::filter_f() {
 return _lpf.freq();
}
  
////////////////////////////////////////////////////////////////////////////////

voices::filter::unsigned_internal_t voices::filter_q() {
 return _lpf.res();
}

////////////////////////////////////////////////////////////////////////////////

void voices::filter_f(voices::filter::unsigned_internal_t const & x) {
  // Serial.print("F: ");
  // Serial.print(x.value);
  // Serial.print(" ");
  // Serial.println();
 
 _lpf.freq(x);
}
  
////////////////////////////////////////////////////////////////////////////////

void voices::filter_q(voices::filter::unsigned_internal_t const & x) {
 // Serial.print("Q: ");
 // Serial.print(x.value);
 // Serial.print(" ");
 // Serial.println();
 
 _lpf.res(x);
}
  
//////////////////////////////////////////////////////////////////////////////

bool voices::volume(voices::volume_type const & volume) {
 if (volume == _volume) return false;
  
 _volume    = volume;
  
 return true;
}

///////////////////////////////////////////////////////////////////////////////

bool voices::pitch(uint8_t const & voice_ix, u0q16::value_type const & parameter) {
 const uint8_t control_shift = 9;
 uint8_t       notes_ix      = parameter >> control_shift;
  
 static int8_t notes[(0xfff >> control_shift) + 1] = {
  0, 2, 3, 5, 7, 8, 10, 12
 };

 item(voice_ix).next_phincr = _phincrs[notes[notes_ix] + BASS_ROOT_NOTE];

  return true;
}

//////////////////////////////////////////////////////////////////////////////

voices::sample voices::read() {
 s15q16 mixed     { 0 };
 auto bass_items  = _items;
 bass_items      += 3;

 MIX_Q(mixed , bass_items, 3);

 if (mixed >= s15q16::ONE) {
  Serial.println("WARNING, attenuate before processing!");
 };

 mixed -= mixed >> 2;
 mixed  =_lpf.process(s0q15(mixed)); 
 
 MIX_Q(mixed, _items,     3);

 mixed *= _volume;

 if (mixed >= s15q16::ONE) {
  Serial.println("WARNING, RED LINE!");

  mixed = s15q16::MAX;
 }
 else if (mixed <= -s15q16::ONE) {
  Serial.println("WARNING, RED LINE!");

  mixed = s15q16::MIN;
 };
 
 return mixed;
}

////////////////////////////////////////////////////////////////////////////////

/* Local Variables:  */
/* fill-column: 100  */
/* End:              */
