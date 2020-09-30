#include "SPI.h"
#include "lamb.h"
#include "samples.h"

const uint32_t TFT_DC        = PA8;
const uint32_t TFT_CS        = PB12;
const uint32_t I2S_WS        = PA3;
const uint32_t I2S_BCK       = PA5;
const uint32_t I2S_DATA      = PA7;
const uint32_t capture_ratio = 3;
const uint32_t ctl_ratio     = 10;
const uint32_t text_ratio    = 10;

Adafruit_ILI9341_STM_SPI2
         tft = Adafruit_ILI9341_STM_SPI2(TFT_CS, TFT_DC);  
uint16_t hData, lData;
uint16_t knob0;
uint16_t knob1;
int16_t  sample;
int16_t  capture[8];
int32_t  avg_sample = 0;
double   pct;
uint32_t delay_ms    = 1;
uint32_t oldDelayMs = delay_ms;

void setup() {
  pinMode(PA0, INPUT);
 
  tft.begin();
  tft.setRotation(3);
  tft.setTextColor(ILI9341_WHITE);  
  tft.setTextSize(2);
  tft.fillScreen(ILI9341_BLACK);
        
  SPI.begin();
  SPI.setBitOrder(MSBFIRST);
  SPI.setDataMode(SPI_MODE0);
  SPI.setClockDivider(SPI_CLOCK_DIV2);

  pinMode(I2S_WS, OUTPUT);
}

void graph() {
  static uint16_t col = 0;
  static const uint16_t col_max = 200; // real max 320
  uint16_t tmp_col = col+120;
  
  tft.drawFastVLine(tmp_col, 0, 240, ILI9341_BLACK);
  uint16_t tmp_knob0 = 119 - map(knob0, 0, 4091, 0, 119);
  tft.drawPixel(tmp_col, tmp_knob0, ILI9341_GREEN);
  tft.drawPixel(tmp_col, 239-tmp_knob0, ILI9341_GREEN);

  int16_t tmp_sample = map(avg_sample, -32768, 32767, -120, 119);
  
  if (tmp_sample > 0)
    tft.drawFastVLine(tmp_col, 120, tmp_sample, ILI9341_YELLOW);
  else 
  if (tmp_sample < 0) {
    tft.drawFastVLine(tmp_col, 120 + tmp_sample, abs(tmp_sample), ILI9341_YELLOW);
  }
  
  col++;
  col %= col_max; 
}

size_t sample_ix = 0;

void audio() {  
  if (((sample_ix % (1 << text_ratio)) == 0) && 
      (oldDelayMs != delay_ms)) {
    draw_text();
  }
  
  if ((sample_ix % (1 << capture_ratio)) == 0) {
    avg_sample >>= capture_ratio;

    graph();

    avg_sample = 0;
  }
  
  if ((sample_ix % (1 << (ctl_ratio))) == 0) {
    uint16_t tmp0 = knob0;
    knob0 <<= 4;
    knob0 -= tmp0;
    knob0 += analogRead(PA0);
    knob0 >>= 4;
    pct = knob0 / 4092.0;

    oldDelayMs = delay_ms;
    uint16_t tmp1 = knob1;
    knob1 <<= 3;
    knob1 -= tmp1;
    knob1 += analogRead(PA1);
    knob1 >>= 3;
    delay_ms = knob1 >> 6;
  }
  
  sample = pct * data[sample_ix];      
  avg_sample += sample;
 
  lData = sample;
  hData = sample;
  hData >>= 8;
  
  digitalWrite(I2S_WS, LOW);
  SPI.transfer(hData);
  SPI.transfer(lData);
  
  digitalWrite(I2S_WS, HIGH);

  sample_ix ++;
  sample_ix %= NUM_ELEMENTS;
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

void loop(void) {
  audio();
}
