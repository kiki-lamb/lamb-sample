#include "SPI.h"
#include "lamb.h"
#include "tracks.h"
#include "samples.h"
#include "application.h"

void setup() {
  for (size_t ix = 0; ix < Tracks::VOICE_COUNT; ix++) {
    Application::voices[ix] = new Application::sample_t(Samples::data+Application::BLOCK_SIZE*ix, Application::BLOCK_SIZE);
  }
  
#ifdef ENABLE_SERIAL
  Serial.begin(115200);
#endif
  
  pinMode(PA0, INPUT);
 
  Application::tft.begin();
  Application::tft.setRotation(3);
  Application::tft.setTextColor(ILI9341_WHITE);  
  Application::tft.setTextSize(2);
  Application::tft.fillScreen(ILI9341_BLACK);
        
  SPI.begin();

  Application::pt8211.begin(&SPI);
  
  lamb::MapleTimer::setup(Application::timer_1, Application::SRATE, srate);
  lamb::MapleTimer::setup(Application::timer_2, Application::KRATE, krate);

  Application::timer_3.pause();
  Application::timer_3.setPeriod(125000);
  Application::timer_3.setChannel1Mode(TIMER_OUTPUT_COMPARE);
  Application::timer_3.setCompare(TIMER_CH1, 0);
  Application::timer_3.attachCompare1Interrupt(lrate);
  Application::timer_3.refresh();    
  Application::timer_3.resume();
}
 
void lrate() {
  for (size_t ix = 0; ix < Tracks::VOICE_COUNT; ix++) {
    if (Tracks::data[Application::lrate_ix % Tracks::NUM_ELEMENTS][ix][0] > 0) {
      Application::voices[ix]->trigger = true;

      Application::voices[ix]->amplitude   = Tracks::data[
        Application::lrate_ix % Tracks::NUM_ELEMENTS
      ][ix][0];
      
      Application::voices[ix]->phase_shift = Tracks::data[
        Application::lrate_ix % Tracks::NUM_ELEMENTS
      ][ix][1];
    }

    Application::draw_flag = true;
  }
  
  static uint32_t count = 0;
  
#ifdef ENABLE_SERIAL
  Serial.print(++count);
  Serial.print(": ");
  Serial.print(Application::total_samples);
  Serial.print(" SRATE: ");
  Serial.print(Application::SRATE);
  Serial.print(" KRATE: ");
  Serial.print(Application::KRATE);
  Serial.println();
#endif

  Application::total_samples = 0;
  Application::lrate_ix ++;
}

const uint32_t v_spacing = 48;

void graph() {
  if (Application::draw_flag) {
    for (size_t ix = 0; ix < Tracks::VOICE_COUNT; ix++) {
      if (Application::voices[ix]->state) {
        Application::tft.fillRect(
          36,
          v_spacing*ix+(v_spacing >> 1),
          v_spacing-10,
          v_spacing-10,
          ILI9341_RED
        );
      }
      else {
        Application::tft.fillRect(
          36,
          v_spacing*ix+(v_spacing >> 1),
          v_spacing-10,
          v_spacing-10,
          ILI9341_BLACK
        );
      }
    }

    Application::draw_flag = false;
  }            
  
  static uint16_t col = 0;
  static const uint16_t col_max = 200; // real max 320
  uint16_t tmp_col = col+120;
  
  Application::tft.drawFastVLine(tmp_col, 0, 240, ILI9341_BLACK);
  uint16_t tmp_knob0 = 119 - map(Application::knob0, 0, 4091, 0, 119);
  Application::tft.drawPixel(tmp_col, tmp_knob0, ILI9341_GREEN);
  Application::tft.drawPixel(tmp_col, 239-tmp_knob0, ILI9341_GREEN);

  int16_t tmp = Application::drawbuff.read();
  int16_t tmp_sample = map(tmp, -32768, 32767, -120, 119);
  
  if (tmp_sample > 0)
    Application::tft.drawFastVLine(
      tmp_col,
      120,
      tmp_sample,
      ILI9341_YELLOW
    );
  else 
  if (tmp_sample < 0) {
    Application::tft.drawFastVLine(
      tmp_col,
      120 + tmp_sample,
      abs(tmp_sample),
      ILI9341_YELLOW);
  }
  
  col++;
  col %= col_max; 
}

void krate() {  
  uint16_t tmp0 = Application::knob0;
  Application::knob0 <<= 4;
  Application::knob0 -= tmp0;
  Application::knob0 += analogRead(PA0);
  Application::knob0 >>= 4;
  Application::pct = Application::knob0 / 4092.0;
}
  
void srate() {
  if ((Application::sample_ix % (1 << Application::CAPTURE_RATIO)) == 0) {
    Application::avg_sample >>= Application::CAPTURE_RATIO;

    if (Application::drawbuff.writable()) {
      Application::drawbuff.write(Application::avg_sample);
    }
    
   Application::avg_sample = 0;
  }
  
  int32_t sample = 0;
  
  for (size_t ix = 0; ix < Tracks::VOICE_COUNT; ix++) {
    sample += Application::voices[ix]->play();
  }

  sample >>= 1;

  sample *= Application::pct;
  
  Application::avg_sample += sample;

  Application::pt8211.write_mono(sample);
  
  Application::total_samples ++;
  Application::sample_ix ++;
  Application::sample_ix %= Samples::NUM_ELEMENTS; 
}

void draw_text() {
  Application::tft.fillRect(10, 10, 80, 80, ILI9341_BLACK);
  Application::tft.setCursor(10, 10);
  Application::tft.println(Application::knob0);
  Application::tft.setCursor(10, 30);
  Application::tft.println(Application::pct);
  Application::tft.setCursor(10, 50);
  Application::tft.println(Application::knob1);
}

void loop(void) {
  if (Application::drawbuff.readable()) {
    graph();
  }
}
