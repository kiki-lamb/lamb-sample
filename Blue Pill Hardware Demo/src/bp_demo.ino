#include "SPI.h"
#include "lamb.h"
#include "tracks.h"
#include "samples.h"
#include "application.h"

using namespace Application;

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

void loop(void) {
  if (drawbuff.readable()) {
    graph();
  }
}
