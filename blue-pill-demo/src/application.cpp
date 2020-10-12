#include "application.h"
#include "samples.h"
#include <inttypes.h>
#include <Arduino.h>

const uint32_t          application::K_RATE        = 100;
const uint32_t          application::S_RATE        = 19000;
uint16_t                application::knob0         = 4091;
uint16_t                application::knob1         = 4091;
uint16_t                application::knob2         = 4091;
int32_t                 application::avg_sample    = 0;
size_t                  application::sample_ix     = 0;
size_t                  application::total_samples = 0;
double                  application::pct           = 100.0;
HardwareTimer           application::timer_1(1);
HardwareTimer           application::timer_2(2);
HardwareTimer           application::timer_3(3);
application::sample_t * application::voices[6];

lamb::device::Adafruit_ILI9341_STM_SPI2 application::tft =
  lamb::device::Adafruit_ILI9341_STM_SPI2(
    application::TFT_CS,
    application::TFT_DC
  );  
  
lamb::device::pt8211 application::pt8211(application::I2S_WS);

lamb::ring_buffer<int16_t, 256> application::drawbuff;

volatile bool application::draw_flag = false;

uint8_t application::last_button_values = 0;
uint8_t application::queued = 0;  

//////////////////////////////////////////////////////////////////////////////

void application::k_rate() {  
  uint16_t tmp0 = knob0;
  knob0 <<= 4;
  knob0 -= tmp0;
  knob0 += analogRead(PA0);
  knob0 >>= 4;
    
  pct = knob0 / 2048.0;
    
  static size_t buttons[] =      {  PB11  ,  PB10 ,    PB1 ,   PB0, PC14, PC15   };
  static char * button_names[] = { "PB11", "PB10",  "PB1",  "PB0", "PC14", "PC15"  };
    
  uint8_t button_values = 0;

  for (size_t ix = 0; ix < 6; ix++) {
    if (! digitalRead(buttons[ix])) {
      button_values |= 1 << ix;
    }
  }

#ifdef LOG_RAW_BUTTONS
  for(uint16_t mask = 0x80; mask; mask >>= 1) {
    if(mask  & last_button_values)
      Serial.print('1');
    else
      Serial.print('0');
  }
    
  Serial.print(" => ");
    
  for(uint16_t mask = 0x80; mask; mask >>= 1) {
    if(mask  & button_values)
      Serial.print('1');
    else
      Serial.print('0');
  }
  Serial.println();
#endif
    
  for (size_t ix = 0; ix < 6; ix++) {
    if ( (! (last_button_values & (1 << ix))) &&
         (button_values & (1 << ix))) {
      Serial.print("Triggered ");
      Serial.print(button_names[ix]);
      
      if ((ix == 2) || (ix == 3)) {
        Serial.print(" ");
      }
      
      Serial.print(" @ 0x");
      Serial.print(voices[ix]->amplitude, HEX);
      Serial.print(" / ");
      Serial.println(ix);
      
      voices[ix]->trigger   = true;
      
      if (ix == 5)
        voices[4]->trigger   = false;
      
      if (ix == 4)
        voices[5]->trigger   = false;
      
    }
  }
    
  last_button_values = button_values;
}

void application::graph() {
  if (draw_flag) {
    for (size_t ix = 0; ix < 6; ix++) {
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

  int16_t tmp = drawbuff.dequeue();
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
  
void application::s_rate() {
  if ((sample_ix % (1 << CAPTURE_RATIO)) == 0) {
    avg_sample >>= CAPTURE_RATIO;

    if (drawbuff.writable()) {
      drawbuff.enqueue(avg_sample); // 
    }
    
    avg_sample = 0;
  }
  
  int32_t sample = 0;
  
  for (size_t ix = 0; ix < 6; ix++) {
    int16_t tmp = voices[ix]->play();
      
    if ((ix == 5) && voices[ix]->trigger) {
      Serial.println(tmp);
    }
      
    sample += tmp;
  }

  sample >>= 1;

  sample *= pct;
  
  avg_sample += sample;

  pt8211.write_mono(sample);
  
  total_samples ++;
  sample_ix ++;
  sample_ix %= Samples::NUM_ELEMENTS; 
}

void application::draw_text() {
  tft.fillRect(10, 10, 80, 80, ILI9341_BLACK);
  tft.setCursor(10, 10);
  tft.println(knob0);
  tft.setCursor(10, 30);
  tft.println(pct);
  tft.setCursor(10, 50);
  tft.println(knob1);
}
  
void application::setup() {
  for (size_t ix = 0; ix < 6; ix++) {
    voices[ix] = new sample_t(Samples::data+BLOCK_SIZE*ix, BLOCK_SIZE);
  }
    
#ifdef ENABLE_SERIAL
  Serial.begin(115200);
#endif
    
  pinMode(PA0, INPUT);
  pinMode(PA1, INPUT);
  pinMode(PA2, INPUT);
    
  tft.begin();
  tft.setRotation(3);
  tft.setTextColor(ILI9341_WHITE);  
  tft.setTextSize(2);
  tft.fillScreen(ILI9341_BLACK);
    
  SPI.begin();

  voices[0]->amplitude = 0xbf; // kick
  voices[1]->amplitude = 0xdf; // lo bass
  voices[2]->amplitude = 0xdf; // hi bass
  voices[3]->amplitude = 0x7f; // snare 
  voices[4]->amplitude = 0xff; // closed hat
  voices[5]->amplitude = 0x9f; // open hat
    
  pt8211.begin(&SPI);
    
  lamb::maple_timer::setup(timer_1, S_RATE, s_rate);
  lamb::maple_timer::setup(timer_2, K_RATE, k_rate);
    
  pinMode(PB0,  INPUT_PULLUP);
  pinMode(PB1,  INPUT_PULLUP);
  pinMode(PB10, INPUT_PULLUP);
  pinMode(PB11, INPUT_PULLUP);
  pinMode(PC14, INPUT_PULLUP);
  pinMode(PC15, INPUT_PULLUP);
    
  delay(500);
}
  
void application::loop(void) {
  if (drawbuff.readable()) {
    graph();
  }
}
