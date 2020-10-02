#ifndef APPLICATION_H
#define APPLICATION_H

#include <inttypes.h>

namespace Application {
  const uint32_t KRATE         = 100;
  const uint32_t SRATE         = 20000;
  const uint32_t TFT_DC        = PA8;
  const uint32_t TFT_CS        = PB12;
  const uint32_t I2S_WS        = PA3;
  const uint32_t I2S_BCK       = PA5;
  const uint32_t I2S_DATA      = PA7;
  const uint32_t CAPTURE_RATIO = 3;
  const size_t   BLOCK_SIZE    = (Samples::NUM_ELEMENTS >> 2);
}

#endif

