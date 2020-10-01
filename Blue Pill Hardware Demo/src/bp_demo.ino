#include "SPI.h"
#include "lamb.h"
#include "samples.h"

const uint32_t KRATE         = 100;
const uint32_t SRATE         = 20000; // 22050;
const uint32_t TFT_DC        = PA8;
const uint32_t TFT_CS        = PB12;
const uint32_t I2S_WS        = PA3;
const uint32_t I2S_BCK       = PA5;
const uint32_t I2S_DATA      = PA7;
const uint32_t capture_ratio = 3;
const size_t   block_size    = (NUM_ELEMENTS >> 2);

uint16_t knob0,
         knob1;
int16_t  sample;
int32_t  avg_sample;
size_t   sample_ix,
         total_samples;
double   pct;

HardwareTimer
         timer_1(1),
         timer_2(2),
         timer_3(3);
Adafruit_ILI9341_STM_SPI2
         tft = Adafruit_ILI9341_STM_SPI2(TFT_CS, TFT_DC);  

lamb::Device::PT8211 pt8211(I2S_WS);
lamb::RingBuffer<int16_t, 256>
         drawbuff;

typedef lamb::oneshot_plus sample_t;

sample_t bd(data+block_size*0, block_size);
sample_t lt(data+block_size*1, block_size);
sample_t sd(data+block_size*2, block_size);
sample_t sy(data+block_size*3, block_size);

void setup() {
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
  lamb::MapleTimer::setup(timer_2, KRATE, krate);

  timer_3.pause();
  timer_3.setPeriod(125000);
  timer_3.setChannel1Mode(TIMER_OUTPUT_COMPARE);
  timer_3.setCompare(TIMER_CH1, 0);
  timer_3.attachCompare1Interrupt(lrate);
  timer_3.refresh();    
  timer_3.resume();
}

uint32_t lrate_ix;


uint8_t bd_track[] = {
  0,    0,    0,    0,
  0,    0,    0,    0,
  0,    0,    0,    0,
  0,    0,    0,    0,

  0,    0,    0,    0,
  0,    0,    0,    0,
  0,    0,    0,    0,
  0,    0,    0,    0,

  0,    0,    0,    0,
  0,    0,    0,    0,
  0,    0,    0,    0,
  0,    0,    0,    0,
  
  0,    0,    0,    0,
  0,    0,    0,    0,
  0,    0,    0,    0x80, 
  0x80, 0,    0xC0, 0,
  
  0xFF, 0,    0,    0,
  0xFF, 0,    0,    0,
  0xFF, 0,    0,    0,
  0xFF, 0,    0,    0,

  0xFF, 0,    0,    0,
  0xFF, 0,    0,    0,
  0xFF, 0,    0,    0,
  0xFF, 0,    0,    0x80,

  0xFF, 0,    0,    0,
  0xFF, 0,    0,    0,
  0xFF, 0,    0,    0,
  0xFF, 0,    0,    0,

  0xFF, 0,    0,    0,
  0xFF, 0,    0,    0,
  0xFF, 0,    0,    0,
  0xFF, 0,    0x80, 0
};

uint8_t sd_track[] = {
  0, 0, 0, 0,
  0, 0, 0, 0,
  0, 0, 0, 0,
  0, 0, 0, 0,
  
  0, 0, 0, 0,
  0, 0, 0, 0,
  0, 0, 0, 0,
  0, 0, 0, 0,

  0, 0, 0, 0,
  0, 0, 0, 0,
  0, 0, 0, 0,
  0, 0, 0, 0,
  
  0, 0, 0, 0,
  0, 0, 0, 0,
  0, 0, 0, 0,
  0, 0, 0, 0,

  0,    0, 0, 0,
  0x40, 0, 0, 0,
  0,    0, 0, 0,
  0x40, 0, 0, 0,

  0,    0, 0, 0,
  0x40, 0, 0, 0,
  0,    0, 0, 0,
  0x40, 0, 0, 0x40,

  0,    0, 0, 0,
  0x40, 0, 0, 0,
  0,    0, 0, 0,
  0x40, 0, 0, 0,

  0,    0, 0, 0,
  0x40, 0, 0, 0,
  0,    0, 0, 0,
  0x40, 0, 0x40, 0,
};


uint8_t sy_track[] = {
  0x00, 0x04, 0x08, 0x0C,
  0x10, 0x14, 0x18, 0x1C,
  0x20, 0x24, 0x28, 0x2C,
  0x30, 0x34, 0x38, 0x3C,

  0x40, 0x44, 0x48, 0x4C,
  0x50, 0x54, 0x58, 0x5C,
  0x60, 0x64, 0x68, 0x6C,
  0x70, 0x74, 0x78, 0x7C,

  0x80, 0x80, 0x80, 0x80,
  0x80, 0x80, 0x80, 0x80,
  0x80, 0x80, 0x80, 0x80,
  0x80, 0x80, 0x80, 0x80, 

  0x80, 0x80, 0x80, 0x80,
  0x80, 0x80, 0x80, 0x80,
  0x80, 0x80, 0x80, 0x80,
  0x80, 0x80, 0x80, 0x80, 

  0x80, 0x80, 0x80, 0x80,
  0x80, 0x80, 0x80, 0x80,
  0x80, 0x80, 0x80, 0x80,
  0x80, 0x80, 0x80, 0x80, 

  0x80, 0x80, 0x80, 0x80,
  0x80, 0x80, 0x80, 0x80,
  0x80, 0x80, 0x80, 0x80,
  0x80, 0x80, 0x80, 0x80, 

  0x80, 0x80, 0x80, 0x80,
  0x80, 0x80, 0x80, 0x80,
  0x80, 0x80, 0x80, 0x80,
  0x80, 0x80, 0x80, 0x80, 

  0x80, 0x80, 0x80, 0x80,
  0x80, 0x80, 0x80, 0x80,
  0x80, 0x80, 0x80, 0x80,
  0x80, 0x80, 0x80, 0x80, 
};

uint8_t sy_shift[] = {
  0, 1, 2, 0,
  1, 2, 0, 1,
  2, 0, 0, 1,
  2, 0, 1, 2
};

uint8_t lt_track[] = { 
  0,    0,    0xC0, 0,
  0,    0,    0,    0,   
  0,    0,    0,    0,   
  0,    0x80, 0,    0
};

void lrate() {
  if (bd_track[lrate_ix % 128] > 0) {
    bd.trigger = true;
    bd.amplitude = bd_track[lrate_ix % 128];
  }

  if (sd_track[lrate_ix % 128] > 0) {
    sd.trigger = true;
    sd.amplitude = sd_track[lrate_ix % 128];
  }

  if (sy_track[lrate_ix % 128] > 0) {
    sy.trigger = true;
    sy.amplitude = sy_track[lrate_ix % 128];
  }
  sy.phase_shift = sy_shift[lrate_ix % 16];
  
//  if (lt_track[lrate_ix % 16] > 0) {
//    lt.trigger = true;
//    lt.amplitude = lt_track[lrate_ix % 16];
//  }
  
  static uint32_t count = 0;
  
#ifdef ENABLE_SERIAL
  Serial.print(++count);
  Serial.print(": ");
  Serial.println(total_samples);
#endif

  total_samples = 0;
  lrate_ix ++;
}

void graph() {
  static uint16_t col = 0;
  static const uint16_t col_max = 200; // real max 320
  uint16_t tmp_col = col+120;
  
  tft.drawFastVLine(tmp_col, 0, 240, ILI9341_BLACK);
  uint16_t tmp_knob0 = 119 - map(knob0, 0, 4091, 0, 119);
  tft.drawPixel(tmp_col, tmp_knob0, ILI9341_GREEN);
  tft.drawPixel(tmp_col, 239-tmp_knob0, ILI9341_GREEN);

  int16_t tmp = drawbuff.read();
  int16_t tmp_sample = map(tmp, -32768, 32767, -120, 119);
// #ifdef ENABLE_SERIAL
//   Serial.print("Read ");
//   Serial.print(tmp);
//   Serial.print(" of ");
//   Serial.println(drawbuff.count());
// #endif
  
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

    if (drawbuff.writable()) {
      drawbuff.write(avg_sample);
    }
    
    avg_sample = 0;
  }

  sample = pct * (
    (bd.play() >> 1) +
    (sd.play() >> 1) +
    (lt.play() >> 1) +
    (sy.play() >> 1)
  ); 
  avg_sample += sample;

  pt8211.write_mono(sample);
  
  total_samples ++;
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
  if (drawbuff.readable()) {
      graph();
  }
}
