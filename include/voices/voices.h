#ifndef BP_DRUM_VOICES_H
#define BP_DRUM_VOICES_H

#include "lamb.h"
#include "samples/samples.h"
// #include "samples/pluck.h"

class voices {
public:
  typedef int16_t                                             sample;
  typedef lamb::oneshot<sample>                               voice;
  typedef typename lamb::sample_type_traits<sample>::mix_type mix_type;

  static const      mix_type             silence            =
    lamb::sample_type_traits<mix_type>::silence;
  
  static const      uint32_t             S_RATE;
  static const      uint8_t              MIDDLE_OCTAVE      = 4;
  static const      uint8_t              ROOT_NOTE          = 46;
  static const      uint8_t              BASS_ROOT_NOTE     = ((uint8_t)(ROOT_NOTE - 0));
  static const      uint8_t              COUNT              = 6;
  static const      size_t               BLOCK_SIZE         =
    Samples::NUM_ELEMENTS / COUNT;

  static            voice *              items[COUNT];
  static constexpr  size_t               MAP[COUNT] = { 0, 3, 5, 1, 1, 1 };

  static            uint32_t             phincrs[120];
  static            uint12_t             _raw_volume;
  static            uint12_t             _scaled_volume;
  
  static            void                 generate_phincrs();
  static            bool                 volume(uint12_t const & volume);
  static            bool                 pitch(
    uint8_t  const & voice_ix,
    uint12_t const & parameter
  );
  
  static            uint12_t             raw_volume();
  static            uint12_t             scaled_volume();
  static            void                 filter_f(uint8_t const & f_);
  static            void                 filter_q(uint8_t const & q_);
  static            uint8_t              filter_f();
  static            uint8_t              filter_q();
  
  static            sample               read();
  static            void                 setup();
  
  static lamb::lowpass_filter lpf;

};

#endif
