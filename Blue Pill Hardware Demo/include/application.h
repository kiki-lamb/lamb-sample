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

  uint16_t knob0;
  uint16_t knob1;
  int32_t  avg_sample;
  size_t   sample_ix;
  size_t   total_samples;
  double   pct;

  HardwareTimer timer_1(1);
  HardwareTimer timer_2(2);
  HardwareTimer timer_3(3);
  
  Adafruit_ILI9341_STM_SPI2 tft =
    Adafruit_ILI9341_STM_SPI2(
      Application::TFT_CS,
      Application::TFT_DC
    );  
  
  lamb::Device::PT8211 pt8211(Application::I2S_WS);
  lamb::RingBuffer<int16_t, 256>
  drawbuff;

}

#endif

