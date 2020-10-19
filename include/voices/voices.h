#ifndef BP_DRUM_VOICES_H
#define BP_DRUM_VOICES_H

#include "lamb.h"
#include "samples/samples.h"

// #include "samples/pluck.h"

////////////////////////////////////////////////////////////////////////////////

class voices {
public:
  typedef lamb::q0n15_t                                       sample;
  typedef lamb::oneshot<sample>                               voice;
  typedef typename lamb::sample_type_traits<sample>::mix_type mix;
  typedef lamb::lowpass_filter<sample>                        filter;
  
  static const      uint32_t             S_RATE;
  static const      uint8_t              COUNT              = 6;
  static const      mix                  SILENCE            =
    lamb::sample_type_traits<mix>::silence;
  
private:
  voices() = default;
  
  static constexpr  size_t               MAP[COUNT]         = { 0, 3, 5, 1, 1, 1 };
  static const      uint8_t              MIDDLE_OCTAVE      = 4;
  static const      uint8_t              ROOT_NOTE          = 46;
  static const      uint8_t              BASS_ROOT_NOTE     = ((uint8_t)(ROOT_NOTE - 0));
  static const      size_t               BLOCK_SIZE         =
    Samples::NUM_ELEMENTS / COUNT;

  static            filter               _lpf;  
  static            lamb::q0n32_t        _phincrs[120];
  static            lamb::q0n12_t        _volume;
  static            lamb::q0n12_t        _scaled_volume;
  static            voice *              _items[COUNT];
  
  static            void                 generate_phincrs();  
  static            voice &              item(size_t const & ix);  
  
public:
  static            void                 trigger(uint8_t const & ix);
  static            lamb::q0n12_t        volume();
  static            void                 filter_f(lamb::q0n8_t const & f_);
  static            void                 filter_q(lamb::q0n8_t const & q_);
  static            lamb::q0n8_t         filter_f();
  static            lamb::q0n8_t         filter_q();  
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
