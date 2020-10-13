#include "application.h"
#include "samples.h"
#include <inttypes.h>
#include <Arduino.h>

using namespace lamb;

const uint32_t               application::K_RATE         = 100;
const uint32_t               application::S_RATE         = 19000;
uint16_t                     application::_knob0         = 4091;
uint16_t                     application::_knob1         = 4091;
uint16_t                     application::_knob2         = 4091;
int32_t                      application::_avg_sample    = 0;
size_t                       application::_sample_ix     = 0;
size_t                       application::_total_samples = 0;
double                       application::_pct           = 100.0;
HardwareTimer                application::_timer_1(1);
HardwareTimer                application::_timer_2(2);
HardwareTimer                application::_timer_3(3);
application::sample_t *      application::_voices[6];
application::dac             application::_dac(application::I2S_WS);
application::draw_buffer     application::_draw_buffer;
uint8_t                      application::_last_button_values = 0;
uint8_t                      application::_queued             = 0;  

application::tft application::_tft(
  application::TFT_CS,
  application::TFT_DC
);  
  
//////////////////////////////////////////////////////////////////////////////

void application::k_rate() {  
  uint16_t tmp0 = _knob0;
  _knob0 <<= 4;
  _knob0 -= tmp0;
  _knob0 += analogRead(PA0);
  _knob0 >>= 4;
    
  _pct = _knob0 / 2048.0;
    
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
    if(mask  & _last_button_values)
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
    if ( (! (_last_button_values & (1 << ix))) &&
         (button_values & (1 << ix))) {
      Serial.print("Triggered ");
      Serial.print(button_names[ix]);
      
      if ((ix == 2) || (ix == 3)) {
        Serial.print(" ");
      }
      
      Serial.print(" @ 0x");
      Serial.print(_voices[ix]->amplitude, HEX);
      Serial.print(" / ");
      Serial.println(ix);
      
      _voices[ix]->trigger   = true;
      
      if (ix == 5)
        _voices[4]->trigger   = false;
      
      if (ix == 4)
        _voices[5]->trigger   = false;
      
    }
  }
    
  _last_button_values = button_values;
}

void application::graph() {
  static uint16_t col = 0;
  static const uint16_t col_max = 200; // real max 320
  uint16_t tmp_col = col+120;
  
  _tft.drawFastVLine(tmp_col, 0, 240, ILI9341_BLACK);
  uint16_t tmp_knob0 = 119 - map(_knob0, 0, 4091, 0, 119);
  _tft.drawPixel(tmp_col, tmp_knob0, ILI9341_GREEN);
  _tft.drawPixel(tmp_col, 239-tmp_knob0, ILI9341_GREEN);

  int16_t tmp = _draw_buffer.dequeue();

  int16_t tmp_sample = map(tmp, -32768, 32767, -120, 119);
  
  if (tmp_sample > 0)
    _tft.drawFastVLine(
      tmp_col,
      120,
      tmp_sample,
      ILI9341_YELLOW
    );
  else 
    if (tmp_sample < 0) {
      _tft.drawFastVLine(
        tmp_col,
        120 + tmp_sample,
        abs(tmp_sample),
        ILI9341_YELLOW);
    }
  
  col++;
  col %= col_max; 
}
  
void application::s_rate() {
  if ((_sample_ix % (1 << CAPTURE_RATIO)) == 0) {
    _avg_sample >>= CAPTURE_RATIO;

    if (_draw_buffer.writable()) {
      _draw_buffer.enqueue(_avg_sample); // 
    }
    
    _avg_sample = 0;
  }
  
  int32_t sample = 0;
  
  for (size_t ix = 0; ix < 6; ix++) {
    int16_t tmp = _voices[ix]->play();
      
    if ((ix == 5) && _voices[ix]->trigger) {
      Serial.println(tmp);
    }
      
    sample += tmp;
  }

  sample >>= 1;

  sample *= _pct;
  
  _avg_sample += sample;

  _dac.write_mono(sample);
  
  _total_samples ++;
  _sample_ix ++;
  _sample_ix %= Samples::NUM_ELEMENTS; 
}

void application::setup() {
  for (size_t ix = 0; ix < 6; ix++) {
    _voices[ix] = new sample_t(Samples::data+BLOCK_SIZE*ix, BLOCK_SIZE);
  }
    
#ifdef ENABLE_SERIAL
  Serial.begin(115200);
#endif
    
  pinMode(PA0, INPUT);
  pinMode(PA1, INPUT);
  pinMode(PA2, INPUT);
    
  _tft.begin();
  _tft.setRotation(3);
  _tft.setTextColor(ILI9341_WHITE);  
  _tft.setTextSize(2);
  _tft.fillScreen(ILI9341_BLACK);
    
  SPI.begin();

  _voices[0]->amplitude = 0xbf; // kick
  _voices[1]->amplitude = 0xdf; // lo bass
  _voices[2]->amplitude = 0xdf; // hi bass
  _voices[3]->amplitude = 0x7f; // snare 
  _voices[4]->amplitude = 0xff; // closed hat
  _voices[5]->amplitude = 0x9f; // open hat
    
  _dac.begin(&SPI);
    
  maple_timer::setup(_timer_1, S_RATE, s_rate);
  maple_timer::setup(_timer_2, K_RATE, k_rate);
    
  pinMode(PB0,  INPUT_PULLUP);
  pinMode(PB1,  INPUT_PULLUP);
  pinMode(PB10, INPUT_PULLUP);
  pinMode(PB11, INPUT_PULLUP);
  pinMode(PC14, INPUT_PULLUP);
  pinMode(PC15, INPUT_PULLUP);
    
  delay(500);
}
  
void application::loop(void) {
  if (_draw_buffer.readable()) {
    graph();
  }
}
