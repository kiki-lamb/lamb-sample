#ifndef BP_DRUM_VOICES_H
#define BP_DRUM_VOICES_H

#include "lamb.h"
#include "samples/samples.h"

// #include "samples/pluck.h"

////////////////////////////////////////////////////////////////////////////////

class voices {
public:
 typedef lamb::s0q15                                          sample_q;
 typedef sample_q::value_type                                 sample;
 typedef lamb::oneshot<sample, sample_q>                      voice;
 typedef typename lamb::sample_type_traits<sample>::mix_type  mix;
 typedef lamb::filters::basic<>                               filter;
  
 static const      uint32_t             S_RATE;
 static const      uint8_t              COUNT              = 6;
 static const      mix                  SILENCE            =
  lamb::sample_type_traits<mix>::silence;
  
private:
 voices() = default;
  
 static constexpr  size_t               MAP[COUNT]         = { 0, 3, 5, 1, 1, 1 };
 static const      uint8_t              MIDDLE_OCTAVE      = 4;
 static const      uint8_t              ROOT_NOTE          = 44;
 static const      uint8_t              BASS_ROOT_NOTE     = ((uint8_t)(ROOT_NOTE + 2));
 static const      size_t               BLOCK_SIZE         =
  Samples::NUM_ELEMENTS / COUNT;

 static            filter               _lpf;  
 static            lamb::u0q32::value_type        _phincrs[120];
 static            lamb::u0q16::value_type        _volume;
 static            voice *              _items[COUNT];
  
 static            void                 generate_phincrs();  
  
public:
 static            voice &              item(size_t const & ix);  
 static            void                 trigger(uint8_t const & ix);
 static            lamb::u0q16::value_type        volume();
 static            void                 filter_f(filter::unsigned_internal_t const & f_);
 static            void                 filter_q(filter::unsigned_internal_t const & q_);
 static            filter::unsigned_internal_t filter_f();
 static            filter::unsigned_internal_t filter_q();  
 static            sample               read();
 static            void                 setup();
 static            bool                 volume(uint12_t const & volume);
 static            bool                 pitch(
  uint8_t  const & voice_ix,
  uint12_t const & parameter
 );
};

#endif

////////////////////////////////////////////////////////////////////////////////

/* Local Variables:  */
/* fill-column: 100  */
/* End:              */
