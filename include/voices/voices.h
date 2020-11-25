#ifndef BP_DRUM_VOICES_H
#define BP_DRUM_VOICES_H

#include "lamb.h"
#include "samples/samples.h"

// #include "samples/pluck.h"

////////////////////////////////////////////////////////////////////////////////

class voices {
public:
 typedef lamb::s0q15                                         sample;
 typedef lamb::oneshot<sample::value_type, sample>           voice;
 typedef lamb::s15q16                                        mix;
 typedef lamb::filters::basic<>                              filter;
 typedef filter::unsigned_internal_t                         filter_arg;
 typedef lamb::u1q15                                         volume_type;
 
 
 static const      uint32_t             S_RATE;
 static const      uint8_t              COUNT              = 6;
  
private:
 voices() = default;
  
 static constexpr  size_t               MAP[COUNT]         { 0, 3, 5, 1, 1, 1 };
 static const      uint8_t              MIDDLE_OCTAVE      = 4;
 static const      uint8_t              ROOT_NOTE          = 44;
 static const      uint8_t              BASS_ROOT_NOTE     = ((uint8_t)(ROOT_NOTE + 2));
 static const      size_t               BLOCK_SIZE         = Samples::NUM_ELEMENTS / COUNT;

 static            filter               _lpf;  
 static            lamb::u0q32          _phincrs[120];
 static            volume_type          _volume;
 static            voice *              _items[COUNT];
  
 static            void                 generate_phincrs();  
  
public:
 static            voice &              item(size_t const & ix);  
 static            void                 trigger(uint8_t const & ix);
 static            volume_type          volume();
 static            void                 filter_f(filter_arg const & f_);
 static            void                 filter_q(filter_arg const & q_);
 static            filter_arg           filter_f();
 static            filter_arg           filter_q();  
 static            sample               read();
 static            void                 setup();
 static            bool                 volume(volume_type const & volume);
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
