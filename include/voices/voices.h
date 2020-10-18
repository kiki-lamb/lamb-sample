#ifndef BP_DRUM_VOICES_H
#define BP_DRUM_VOICES_H

#include "lamb.h"

class voices {
public:
  typedef int16_t                                            sample;
  typedef lamb::oneshot<sample>                              voice;
};

#endif
