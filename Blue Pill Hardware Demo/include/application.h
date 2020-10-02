#ifndef APPLICATION_H
#define APPLICATION_H

#include <inttypes.h>
#include "tracks.h"
#include "samples.h"

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
  const uint32_t V_SPACING     = 48;

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

  uint32_t lrate_ix;

  volatile bool draw_flag = false;

  typedef lamb::oneshot_plus sample_t;

  sample_t * voices[4];

  //////////////////////////////////////////////////////////////////////////////

  
  void krate() {  
    uint16_t tmp0 = knob0;
    knob0 <<= 4;
    knob0 -= tmp0;
    knob0 += analogRead(PA0);
    knob0 >>= 4;
    pct = knob0 / 4092.0;
  }

  void graph() {
  if (draw_flag) {
    for (size_t ix = 0; ix < Tracks::VOICE_COUNT; ix++) {
      if (voices[ix]->state) {
        tft.fillRect(
          36,
          V_SPACING*ix+(V_SPACING >> 1),
          V_SPACING-10,
          V_SPACING-10,
          ILI9341_RED
        );
      }
      else {
        tft.fillRect(
          36,
          V_SPACING*ix+(V_SPACING >> 1),
          V_SPACING-10,
          V_SPACING-10,
          ILI9341_BLACK
        );
      }
    }

    draw_flag = false;
  }            
  
  static uint16_t col = 0;
  static const uint16_t col_max = 200; // real max 320
  uint16_t tmp_col = col+120;
  
  tft.drawFastVLine(tmp_col, 0, 240, ILI9341_BLACK);
  uint16_t tmp_knob0 = 119 - map(knob0, 0, 4091, 0, 119);
  tft.drawPixel(tmp_col, tmp_knob0, ILI9341_GREEN);
  tft.drawPixel(tmp_col, 239-tmp_knob0, ILI9341_GREEN);

  int16_t tmp = drawbuff.read();
  int16_t tmp_sample = map(tmp, -32768, 32767, -120, 119);
  
  if (tmp_sample > 0)
    tft.drawFastVLine(
      tmp_col,
      120,
      tmp_sample,
      ILI9341_YELLOW
    );
  else 
  if (tmp_sample < 0) {
    tft.drawFastVLine(
      tmp_col,
      120 + tmp_sample,
      abs(tmp_sample),
      ILI9341_YELLOW);
  }
  
  col++;
  col %= col_max; 
}
  
  void srate() {
    if ((sample_ix % (1 << CAPTURE_RATIO)) == 0) {
      avg_sample >>= CAPTURE_RATIO;

      if (drawbuff.writable()) {
        drawbuff.write(avg_sample);
      }
    
      avg_sample = 0;
    }
  
    int32_t sample = 0;
  
    for (size_t ix = 0; ix < Tracks::VOICE_COUNT; ix++) {
      sample += voices[ix]->play();
    }

    sample >>= 1;

    sample *= pct;
  
    avg_sample += sample;

    pt8211.write_mono(sample);
  
    total_samples ++;
    sample_ix ++;
    sample_ix %= Samples::NUM_ELEMENTS; 
  }

  void draw_text() {
    tft.fillRect(10, 10, 80, 80, ILI9341_BLACK);
    tft.setCursor(10, 10);
    tft.println(knob0);
    tft.setCursor(10, 30);
    tft.println(pct);
    tft.setCursor(10, 50);
    tft.println(knob1);
  }

  void lrate() {
    for (size_t ix = 0; ix < Tracks::VOICE_COUNT; ix++) {
      if (Tracks::data[lrate_ix % Tracks::NUM_ELEMENTS][ix][0] > 0) {
        voices[ix]->trigger = true;
        
        voices[ix]->amplitude   = Tracks::data[
          lrate_ix % Tracks::NUM_ELEMENTS
        ][ix][0];
        
        voices[ix]->phase_shift = Tracks::data[
          lrate_ix % Tracks::NUM_ELEMENTS
        ][ix][1];
      }
      
      draw_flag = true;
    }
    
    static uint32_t count = 0;
    
#ifdef ENABLE_SERIAL
    Serial.print(++count);
    Serial.print(": ");
    Serial.print(total_samples);
    Serial.print(" SRATE: ");
    Serial.print(SRATE);
    Serial.print(" KRATE: ");
    Serial.print(KRATE);
    Serial.println();
#endif
    
    total_samples = 0;
    lrate_ix ++;
  }


  void setup() {
    for (size_t ix = 0; ix < Tracks::VOICE_COUNT; ix++) {
      voices[ix] = new sample_t(Samples::data+BLOCK_SIZE*ix, BLOCK_SIZE);
    }
  
#ifdef ENABLE_SERIAL
    Serial.begin(115200);
#endif
  
    pinMode(PA0, INPUT);
 
    tft.begin();
    tft.setRotation(3);
    tft.setTextColor(ILI9341_WHITE);  
    tft.setTextSize(2);
    tft.fillScreen(ILI9341_BLACK);
        
    SPI.begin();

    pt8211.begin(&SPI);
  
    lamb::MapleTimer::setup(timer_1, SRATE, srate);
    lamb::MapleTimer::setup(
      timer_2,
      KRATE,
      krate
    );

    timer_3.pause();
    timer_3.setPeriod(125000);
    timer_3.setChannel1Mode(TIMER_OUTPUT_COMPARE);
    timer_3.setCompare(TIMER_CH1, 0);
    timer_3.attachCompare1Interrupt(lrate);
    timer_3.refresh();    
    timer_3.resume();
  }
 
  void loop(void) {
    if (drawbuff.readable()) {
      graph();
    }
  }
}

#endif

