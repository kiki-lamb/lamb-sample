#include "SPI.h"
#include "lamb.h"
#include "samples.h"

const uint32_t KRATE         = 100;
const uint32_t SRATE         = 22050;
const uint32_t TFT_DC        = PA8;
const uint32_t TFT_CS        = PB12;
const uint32_t I2S_WS        = PA3;
const uint32_t I2S_BCK       = PA5;
const uint32_t I2S_DATA      = PA7;
const uint32_t capture_ratio = 4;

HardwareTimer
         timer_1(1),
         timer_2(2);
Adafruit_ILI9341_STM_SPI2
         tft = Adafruit_ILI9341_STM_SPI2(TFT_CS, TFT_DC);  
uint16_t hData,
         lData,
         knob0,
         knob1;
int16_t  sample;
int32_t  avg_sample;
size_t   sample_ix;
double   pct;

void setup() {
  Serial.begin(115200);
  
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
  
  lamb::MapleTimer::setup(timer_1, SRATE, srate);
  lamb::MapleTimer::setup(timer_2, KRATE, krate);
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
  uint16_t tmp0 = knob0;
  knob0 <<= 4;
  knob0 -= tmp0;
  knob0 += analogRead(PA0);
  knob0 >>= 4;
  pct = knob0 / 4092.0;
}
  
void srate() {
  if ((sample_ix % (1 << capture_ratio)) == 0) {
    avg_sample >>= capture_ratio;
    // graph();
    avg_sample = 0;
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
  Serial.print("Prescale: ");
  Serial.println(lamb::MapleTimer::prescale_from_frequency(22050));
  delay(1000);
}
