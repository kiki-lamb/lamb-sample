#include "SPI.h"
#include "lamb.h"
#include "tracks.h"
#include "samples.h"
#include "application.h"

HardwareTimer timer_1(1);
HardwareTimer timer_2(2);
HardwareTimer timer_3(3);

Adafruit_ILI9341_STM_SPI2
         tft = Adafruit_ILI9341_STM_SPI2(
           Application::TFT_CS,
           Application::TFT_DC
         );  

lamb::Device::PT8211 pt8211(Application::I2S_WS);
lamb::RingBuffer<int16_t, 256>
         drawbuff;

typedef lamb::oneshot_plus sample_t;

sample_t * voices[4];
void setup() {
  for (size_t ix = 0; ix < Tracks::VOICE_COUNT; ix++) {
   voices[ix] = new sample_t(Samples::data+Application::BLOCK_SIZE*ix, Application::BLOCK_SIZE);
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
  
  lamb::MapleTimer::setup(timer_1, Application::SRATE, srate);
  lamb::MapleTimer::setup(timer_2, Application::KRATE, krate);

  timer_3.pause();
  timer_3.setPeriod(125000);
  timer_3.setChannel1Mode(TIMER_OUTPUT_COMPARE);
  timer_3.setCompare(TIMER_CH1, 0);
  timer_3.attachCompare1Interrupt(lrate);
  timer_3.refresh();    
  timer_3.resume();
}
 
uint32_t lrate_ix;

volatile bool draw_flag = false;

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
  Serial.print(Application::total_samples);
  Serial.print(" SRATE: ");
  Serial.print(Application::SRATE);
  Serial.print(" KRATE: ");
  Serial.print(Application::KRATE);
  Serial.println();
#endif

  Application::total_samples = 0;
  lrate_ix ++;
}

const uint32_t v_spacing = 48;

void graph() {
  if (draw_flag) {
    for (size_t ix = 0; ix < Tracks::VOICE_COUNT; ix++) {
      if (voices[ix]->state) {
        tft.fillRect(
          36,
          v_spacing*ix+(v_spacing >> 1),
          v_spacing-10,
          v_spacing-10,
          ILI9341_RED
        );
      }
      else {
        tft.fillRect(
          36,
          v_spacing*ix+(v_spacing >> 1),
          v_spacing-10,
          v_spacing-10,
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
  uint16_t tmp_knob0 = 119 - map(Application::knob0, 0, 4091, 0, 119);
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

    if (drawbuff.writable()) {
      drawbuff.write(Application::avg_sample);
    }
    
   Application::avg_sample = 0;
  }
  
  int32_t sample = 0;
  
  for (size_t ix = 0; ix < Tracks::VOICE_COUNT; ix++) {
    sample += voices[ix]->play();
  }

  sample >>= 1;

  sample *= Application::pct;
  
  Application::avg_sample += sample;

  pt8211.write_mono(sample);
  
  Application::total_samples ++;
  Application::sample_ix ++;
  Application::sample_ix %= Samples::NUM_ELEMENTS; 
}

void draw_text() {
  tft.fillRect(10, 10, 80, 80, ILI9341_BLACK);
  tft.setCursor(10, 10);
  tft.println(Application::knob0);
  tft.setCursor(10, 30);
  tft.println(Application::pct);
  tft.setCursor(10, 50);
  tft.println(Application::knob1);
}

void loop(void) {
  if (drawbuff.readable()) {
    graph();
  }
}
