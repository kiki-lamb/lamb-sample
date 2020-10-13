#include "application/application.h"
#include <inttypes.h>
#include <Arduino.h>

using namespace lamb;

const uint32_t               application::K_RATE               { 100   };
const uint32_t               application::S_RATE               { 19000 };

double                       application::_master_vol          { 100.0 };
int32_t                      application::_avg_sample          { 0     };
size_t                       application::_sample_ix           { 0     };

uint16_t                     application::_knob0               { 4091  };
uint16_t                     application::_knob1               { 4091  };
uint16_t                     application::_knob2               { 4091  };

uint8_t                      application::_last_trigger_states { 0     };

HardwareTimer                application::_timer_1             ( 1     );
HardwareTimer                application::_timer_2             ( 2     );
HardwareTimer                application::_timer_3             ( 3     );

application::voice *         application::_voices              [ 6     ];
application::draw_buffer     application::_draw_buffer;

application::button          application::_button_device0;
application::button          application::_button_device1;
application::button          application::_button_device2;
application::button          application::_button_device3;
application::button          application::_button_device4;
application::button          application::_button_device5;

application::button_source   application::_button_source0(&_button_device0, 0);
application::button_source   application::_button_source1(&_button_device1, 1);
application::button_source   application::_button_source2(&_button_device2, 2);
application::button_source   application::_button_source3(&_button_device3, 3);
application::button_source   application::_button_source4(&_button_device4, 4);
application::button_source   application::_button_source5(&_button_device5, 5);

application::combined_source application::_combined_source;

application::control_source
application::_control_event_source;

application::dac             application::_dac(
  application::I2S_WS
);

application::tft             application::_tft(
  application::TFT_CS,
  application::TFT_DC
); 
  
//////////////////////////////////////////////////////////////////////////////

application::application_event application::process_control_event(
  application::control_event const & control_event
) {
  application_event application_event;
  application_event.type = application_event_type::EVT_UNKNOWN;
 
  if (control_event.type == control_event_type::CTL_EVT_NOT_AVAILABLE) {
    application_event.type = application_event_type::APP_EVT_NOT_AVAILABLE;

    return application_event;
  }
  else if (control_event.type == control_event_type::EVT_BUTTON) {
    return process_button_event(control_event);
  }
//  else if (control_event.type == control_event_type::EVT_ENCODER) {
//    return process_encoder_event(control_event);
//  }

  return application_event;
}

////////////////////////////////////////////////////////////////////////////////


application::application_event application::process_button_event(
  application::control_event const & control_event
) {
  application_event application_event;
  application_event.type           = application_event_type::EVT_UNKNOWN;
  uint8_t           button_number  = control_event.parameter_hi();
  int8_t            button_state   = (int8_t)control_event.parameter_lo(); 
  
  Serial.print(F("Button event, number: "));
  Serial.print(button_number);
  Serial.print(F(", state: "));
  Serial.print(button_state);
  Serial.println();

  application_event.type      = application_event_type::EVT_TRIGGER;
  application_event.parameter = button_number;
  
  return application_event;
}

//////////////////////////////////////////////////////////////////////////////

void application::k_rate() {
  uint16_t tmp0 = _knob0;
  _knob0 <<= 4;
  _knob0 -= tmp0;
  _knob0 += analogRead(PA0);
  _knob0 >>= 4;
    
  _master_vol = _knob0 / 2048.0;
    
  static size_t buttons[] =      {  PB11  ,  PB10 ,    PB1 ,   PB0, PC14, PC15   };
  static char * button_names[] = { "PB11", "PB10",  "PB1",  "PB0", "PC14", "PC15"  };
    
  uint8_t trigger_states = 0;

  _control_event_source.poll();

  while(
    application_event ae = process_control_event(
      _control_event_source.dequeue_event()
    )
  ) {
    trigger_states |= 1 << ae.parameter;
  }
  
//  }

#ifdef LOG_RAW_BUTTONS
  for(uint16_t mask = 0x80; mask; mask >>= 1) {
    if(mask  & _last_trigger_states)
      Serial.print('1');
    else
      Serial.print('0');
  }
    
  Serial.print(" => ");
    
  for(uint16_t mask = 0x80; mask; mask >>= 1) {
    if(mask  & trigger_states)
      Serial.print('1');
    else
      Serial.print('0');
  }
  Serial.println();
#endif
    
  for (size_t ix = 0; ix < 6; ix++) {
    if ( (! (_last_trigger_states & (1 << ix))) &&
         (trigger_states & (1 << ix))) {
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
    
  _last_trigger_states = trigger_states;
}

void application::graph() {
  static uint16_t col = 0;
  static const uint16_t col_max = 200; // real max 320
  uint16_t tmp_col = col+120;
  
  _tft.drawFastVLine(tmp_col, 0, 240, ILI9341_BLACK);
  uint16_t tmp_knob0 = 119 - map(_knob0, 0, 4091, 0, 119);
  _tft.drawPixel(tmp_col, tmp_knob0, ILI9341_GREEN);
  _tft.drawPixel(tmp_col, 239-tmp_knob0, ILI9341_GREEN);

  sample tmp = _draw_buffer.dequeue();

  sample tmp_sample = map(tmp, -32768, 32767, -120, 119);
  
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
  
  int32_t sample_ = 0;
  
  for (size_t ix = 0; ix < 6; ix++) {
    sample tmp = _voices[ix]->play();
      
    if ((ix == 5) && _voices[ix]->trigger) {
      Serial.println(tmp);
    }
      
    sample_ += tmp;
  }

  sample_ >>= 1;

  sample_ *= _master_vol;
  
  _avg_sample += sample_;

  _dac.write_mono(sample_);
  
  _sample_ix ++;
  _sample_ix %= Samples::NUM_ELEMENTS; 
}

void application::setup_controls() {
  _button_device0      .setup(PB0);
  _button_device1      .setup(PB1);
  _button_device2      .setup(PB10);
  _button_device3      .setup(PB11);
  _button_device4      .setup(PC14);
  _button_device5      .setup(PC15);
  _combined_source     .sources[0] = &_button_source0;
  _combined_source     .sources[1] = &_button_source1;
  _combined_source     .sources[2] = &_button_source2;
  _combined_source     .sources[3] = &_button_source3;
  _combined_source     .sources[4] = &_button_source4;
  _combined_source     .sources[5] = &_button_source5;
  _control_event_source.source = &_combined_source;

  pinMode(PA0, INPUT);
  pinMode(PA1, INPUT);
  pinMode(PA2, INPUT);  
}

void application::setup_voices() {
  for (size_t ix = 0; ix < 6; ix++) {
    _voices[ix] = new voice(Samples::data+BLOCK_SIZE*ix, BLOCK_SIZE);
  }

  _voices[0]->amplitude = 0xbf; // kick
  _voices[1]->amplitude = 0xdf; // lo bass
  _voices[2]->amplitude = 0xdf; // hi bass
  _voices[3]->amplitude = 0x7f; // snare 
  _voices[4]->amplitude = 0xff; // closed hat
  _voices[5]->amplitude = 0x9f; // open hat
}
      
void application::setup_tft() {
  _tft.begin();
  _tft.setRotation(3);
  _tft.setTextColor(ILI9341_WHITE);  
  _tft.setTextSize(2);
  _tft.fillScreen(ILI9341_BLACK);
}

void application::setup_dac() {
  SPI.begin();
    
  _dac.begin(&SPI);
}

void application::setup_timers() {
  maple_timer::setup(_timer_1, S_RATE, s_rate);
  maple_timer::setup(_timer_2, K_RATE, k_rate);
}

void application::setup() {
#ifdef ENABLE_SERIAL
  Serial.begin(115200);
#endif
  
  setup_voices();
  setup_controls();
  setup_tft();
  setup_dac();
  setup_timers();
        
  delay(500);
}
  
void application::loop(void) {
  if (_draw_buffer.readable()) {
    graph();
  }
}
