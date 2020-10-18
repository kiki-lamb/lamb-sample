#ifndef BP_DRUM_VOICES_H
#define BP_DRUM_VOICES_H

#include "lamb.h"
#include "samples/samples.h"
// #include "samples/pluck.h"

class voices {
public:
  typedef int16_t                                             sample;
  typedef lamb::oneshot<sample>                               voice;

  static            uint32_t             phincrs[120];

  static const      uint8_t              COUNT              = 6;
  static const      size_t               BLOCK_SIZE         =
    Samples::NUM_ELEMENTS / COUNT;

  static constexpr  size_t               MAP[COUNT] = { 0, 3, 5, 1, 1, 1 };

};

#endif
