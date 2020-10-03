#include "application.h"

#include "tracks.h"
#include "samples.h"
#include <inttypes.h>
#include <Arduino.h>

namespace Application {
  const uint32_t K_RATE        = 100;
  const uint32_t S_RATE        = 16000;
  const uint32_t TFT_DC        = PA8;
  const uint32_t TFT_CS        = PB12;
  const uint32_t I2S_WS        = PA3;
  const uint32_t I2S_BCK       = PA5;
  const uint32_t I2S_DATA      = PA7;
  const uint32_t CAPTURE_RATIO = 3;
  const size_t   BLOCK_SIZE    = (Samples::NUM_ELEMENTS >> 2);
  const uint32_t V_SPACING     = 48;

  enum mode_t {
    MODE_AUTO = 1,
    MODE_CLOCKED = 2,
    MODE_TRIGGERED = 4,
    MODE_PLAYALONG = 8 ,
    MODE_QUANTIZE = 16
  };
  
  mode_t mode = MODE_PLAYALONG;
  
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

  uint32_t l_rate_ix;

  volatile bool draw_flag = false;

  typedef lamb::oneshot_plus sample_t;

  sample_t * voices[4];

  uint8_t last_button_values = 0;
  uint8_t queued = 0;
  
//////////////////////////////////////////////////////////////////////////////

  void k_rate() {  
    uint16_t tmp0 = knob0;
    knob0 <<= 4;
    knob0 -= tmp0;
    knob0 += analogRead(PA0);
    knob0 >>= 4;

    pct = knob0 / (4092.0); // + 256.0);

    uint16_t tmp_knob1 = knob1;
    uint16_t tmp1 = tmp_knob1;
    tmp_knob1 <<= 2;
    tmp_knob1 -= tmp1;
    tmp_knob1 += analogRead(PA1);
    tmp_knob1 >>= 2;
    
    if (tmp_knob1 != knob1) {
      knob1 = tmp_knob1;
      //Serial.print("knob1: ");
      //Serial.print(knob1);
    }

    if (knob1 < 1536) {
      mode = MODE_TRIGGERED;
    }
    else if (knob1 > 3072) {
      mode = MODE_QUANTIZE;
    }
    else {
      mode = MODE_PLAYALONG;
    }

    //Serial.print(" ");
    //Serial.print(mode);

    //Serial.println();
    
    static size_t buttons[] = { PB0, PB1, PB10, PB11 };
    
    uint8_t button_values = 0;
    
    for (size_t ix = 0; ix < 4; ix++) {
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
    
    switch (mode) {
    case MODE_CLOCKED:
      if ( (! (last_button_values & 1)) &&
           (button_values & 1)) {
        //Serial.println("Clock!");
        clock();
      }
      break;
    case MODE_TRIGGERED:
    case MODE_PLAYALONG:
      for (size_t ix = 0; ix < 4; ix++) {
        if ( (! (last_button_values & (1 << ix))) &&
             (button_values & (1 << ix))) {
          Serial.print("Trigger ");
          Serial.println(ix);
          
          voices[3-ix]->trigger = true;
        }
      }
      break;
    case MODE_QUANTIZE:
      for (size_t ix = 0; ix < 4; ix++) {
        if ( (! (last_button_values & (1 << ix))) &&
             (button_values & (1 << ix))) {
          Serial.print("Catch ");
          Serial.println(ix);
          
          queued |= 1 << (3-ix);
        }
      }
      break;
    }
    
    last_button_values = button_values;
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
  
  void s_rate() {
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

  void l_rate () {
    if (!((mode == MODE_AUTO) || (mode == MODE_PLAYALONG) || (mode == MODE_QUANTIZE))) {
      l_rate_ix = 0;
      return;
    } 
    
    clock();
  }

  uint8_t data[Tracks::NUM_ELEMENTS][Tracks::VOICE_COUNT][2] = {
    { { 0xC0, 0 }, { 0x00, 0 }, { 0x00, 0 }, { 0x00, 0 } },
    { { 0x00, 0 }, { 0x00, 0 }, { 0x00, 0 }, { 0x00, 0 } },
    { { 0x00, 0 }, { 0x00, 0 }, { 0x00, 0 }, { 0x00, 0 } },
    { { 0x60, 0 }, { 0x00, 0 }, { 0x00, 0 }, { 0x00, 0 } },
    //-----------------------------------------------------------
    { { 0x00, 0 }, { 0x00, 0 }, { 0xA0, 0 }, { 0x00, 0 } },
    { { 0x00, 0 }, { 0x00, 0 }, { 0x00, 0 }, { 0x00, 0 } },
    { { 0x30, 0 }, { 0x00, 0 }, { 0x00, 0 }, { 0x00, 0 } },
    { { 0x00, 0 }, { 0x00, 0 }, { 0x00, 0 }, { 0x00, 0 } },
    //-----------------------------------------------------------
    { { 0x18, 0 }, { 0x00, 0 }, { 0x00, 0 }, { 0x00, 0 } },
    { { 0x00, 0 }, { 0x00, 0 }, { 0x00, 0 }, { 0x00, 0 } },
    { { 0xC0, 0 }, { 0x00, 0 }, { 0x00, 0 }, { 0x00, 0 } },
    { { 0x0E, 0 }, { 0x00, 0 }, { 0x00, 0 }, { 0x00, 0 } },
    //-----------------------------------------------------------
    { { 0x60, 0 }, { 0x00, 0 }, { 0xA0, 0 }, { 0x00, 0 } },
    { { 0x00, 0 }, { 0x00, 0 }, { 0x00, 0 }, { 0x00, 0 } },
    { { 0x00, 0 }, { 0x00, 0 }, { 0x00, 0 }, { 0x00, 0 } },
    { { 0x30, 0 }, { 0x00, 0 }, { 0x00, 0 }, { 0x00, 0 } },
    //-----------------------------------------------------------

  };

  void print_bits(uint8_t byte) {
    for(uint16_t mask = 0x80; mask; mask >>= 1) {
      if(mask  & byte)
        Serial.print('1');
      else
        Serial.print('0');
    }
  }
  
  void clock() {
      
    for (size_t ix = 0; ix < Tracks::VOICE_COUNT; ix++) {
      Serial.print("Looking for ");
      print_bits(1 << ix);
      Serial.print(" in ");
      print_bits(queued);
      Serial.println();

      if (queued & (1 << ix)) {
        data[
          l_rate_ix % Tracks::NUM_ELEMENTS
        ][ix][0] = 0xff;

        Serial.print("Record #");
        Serial.print(ix);
        Serial.print(" on step ");
        Serial.print(l_rate_ix % Tracks::NUM_ELEMENTS);
        Serial.println();
        
//        voices[ix]->trigger = true;
      }
      
      if (data[l_rate_ix % Tracks::NUM_ELEMENTS][ix][0] > 0) {
        voices[ix]->trigger = true;
          
        voices[ix]->amplitude   = data[
          l_rate_ix % Tracks::NUM_ELEMENTS
        ][ix][0];
        
        voices[ix]->phase_shift = data[
          l_rate_ix % Tracks::NUM_ELEMENTS
        ][ix][1];
      }
    }

    queued = 0;
    
    draw_flag = true;
    
    static uint32_t count = 0;
    
#ifdef ENABLE_SERIAL
//    Serial.print(++count);
//    Serial.print(": ");
//    Serial.print(total_samples);
//    Serial.print(" S_RATE: ");
//    Serial.print(S_RATE);
//    Serial.print(" K_RATE: ");
//    Serial.print(K_RATE);
//    Serial.println();
#endif
    
    total_samples = 0;
    l_rate_ix ++;
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
  
    lamb::MapleTimer::setup(timer_1, S_RATE, s_rate);
    lamb::MapleTimer::setup(
      timer_2,
      K_RATE,
      k_rate
    );

    timer_3.pause();
    timer_3.setPeriod(133000);
    timer_3.setChannel1Mode(TIMER_OUTPUT_COMPARE);
    timer_3.setCompare(TIMER_CH1, 0);
    timer_3.attachCompare1Interrupt(l_rate);
    timer_3.refresh();    
    timer_3.resume();
  
    pinMode(PB0,  INPUT_PULLUP);
    pinMode(PB1,  INPUT_PULLUP);
    pinMode(PB10, INPUT_PULLUP);
    pinMode(PB11, INPUT_PULLUP);
    delay(500);
  }
 
  void loop(void) {
    if (drawbuff.readable()) {
      graph();
    }
  }
}

