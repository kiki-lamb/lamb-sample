#include "application/application.h"
#include <inttypes.h>
#include <Arduino.h>

using namespace lamb;

// const size_t                 application::_voices_map 
  
const uint32_t               application::K_RATE               { 100    };
const uint32_t               application::S_RATE               { 19000  };

double                       application::_master_vol          { 100.0  };
int32_t                      application::_avg_sample          { 0      };
size_t                       application::_sample_ix           { 0      };

uint16_t                     application::_knob0               { 4091   };
uint16_t                     application::_knob1               { 4091   };
uint16_t                     application::_knob2               { 4091   };

HardwareTimer                application::_timer_1             ( 1      );
HardwareTimer                application::_timer_2             ( 2      );
HardwareTimer                application::_timer_3             ( 3      );

application::voice *         application::_voices              [ 6      ];
application::draw_buffer     application::_draw_buffer;
application::combined_source application::_combined_source;
application::signal          application::_signal_device0      (24, 32 );
application::button          application::_button_device0;
application::button          application::_button_device1;
application::button          application::_button_device2;
application::button          application::_button_device3;
application::button          application::_button_device4;
application::button          application::_button_device5;
application::signal_source   application::_signal_source0(&_signal_device0, 2);
application::button_source   application::_button_source0(&_button_device0, 0);
application::button_source   application::_button_source1(&_button_device1, 1);
application::button_source   application::_button_source2(&_button_device2, 2);
application::button_source   application::_button_source3(&_button_device3, 3);
application::button_source   application::_button_source4(&_button_device4, 4);
application::button_source   application::_button_source5(&_button_device5, 5);
application::control_source  application::_control_event_source;
application::dac             application::_dac(application::I2S_WS);
application::tft             application::_tft(application::TFT_CS, application::TFT_DC);

//////////////////////////////////////////////////////////////////////////////

void application::setup_controls() {
  _signal_device0      .setup(PA0);  
  _button_device0      .setup(PB11);
  _button_device1      .setup(PB10);
  _button_device2      .setup(PB1);
  _button_device3      .setup(PB0);
  _button_device4      .setup(PC15);
  _button_device5      .setup(PC14);

  _combined_source     .sources[0]  = &_signal_source0;
//  _combined_source     .sources[0]  = &_button_source0;
  
  _combined_source     .sources[1]  = &_button_source0;
  _combined_source     .sources[2]  = &_button_source1;
  _combined_source     .sources[3]  = &_button_source2;
  _combined_source     .sources[4]  = &_button_source3;
  _combined_source     .sources[5]  = &_button_source4;
  _combined_source     .sources[6]  = &_button_source5;
  _control_event_source.source      = &_combined_source;

  while (false) { // true) {
    _control_event_source.poll();

    auto e = _control_event_source.dequeue_event();

    if (0 != e.type) {
      Serial.print("Got ");
      Serial.print(e.type, HEX);
      Serial.print(" ");
      Serial.print(e.parameter);
      Serial.println();
    }
    
    delay(2);
  }

//  pinMode(PA0, INPUT);
//  pinMode(PA1, INPUT);
//  pinMode(PA2, INPUT);  
}

////////////////////////////////////////////////////////////////////////////////

void application::setup_voices() {
  for (size_t ix = 0; ix < 6; ix++) {
    _voices[_voices_map[ix]] = new voice(Samples::data+BLOCK_SIZE*ix, BLOCK_SIZE);
  }

  _voices[_voices_map[0]]->amplitude = 0xd0; // 0xb8; // kick
  _voices[_voices_map[1]]->amplitude = 0xe0; // 0xd8; // lo bass
  _voices[_voices_map[2]]->amplitude = 0xe0; // 0xd8; // hi bass
  _voices[_voices_map[3]]->amplitude = 0x60; // 0x78; // snare 
  _voices[_voices_map[4]]->amplitude = 0xa0; // closed hat
  _voices[_voices_map[5]]->amplitude = 0x80; // open hat
}

////////////////////////////////////////////////////////////////////////////////
      
void application::setup_tft() {
  _tft.begin();
  _tft.setRotation(3);
  _tft.setTextColor(ILI9341_WHITE);  
  _tft.setTextSize(2);
  _tft.fillScreen(ILI9341_BLACK);
}

////////////////////////////////////////////////////////////////////////////////


void application::setup_dac() {
  SPI.begin();
    
  _dac.begin(&SPI);
}

////////////////////////////////////////////////////////////////////////////////

void application::setup_timers() {
  maple_timer::setup(_timer_1, S_RATE, s_rate);
  maple_timer::setup(_timer_2, K_RATE, k_rate);
}

////////////////////////////////////////////////////////////////////////////////

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
  else if (control_event.type == control_event_type::EVT_SIGNAL) {
//    Serial.print("SIGNAL!");
    Serial.println();
    application_event.type = application_event_type::APP_EVT_NOT_AVAILABLE;

    Serial.println(control_event.parameter &0xfff);
    _master_vol = (control_event.parameter &0xfff) / 2048.0; // 0.75; // _knob0 / 2048.0;
  
  
    return application_event;
  }

  return application_event;
}

////////////////////////////////////////////////////////////////////////////////

application::application_event application::process_encoder_event(
  application::control_event const & control_event
) {
  application_event application_event;
  application_event.type           = application_event_type::EVT_UNKNOWN;

  Serial.println("Don't know how to process encoders yet!");

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

////////////////////////////////////////////////////////////////////////////////

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
  
  col ++;
  col %= col_max; 
}

//////////////////////////////////////////////////////////////////////////////

void application::k_rate() {
//  uint16_t tmp0 = _knob0;
//  _knob0 <<= 4;
//  _knob0 -= tmp0;
//  _knob0 += analogRead(PA0);
//  _knob0 >>= 4;
    
//  _master_vol = 0.75; // _knob0 / 2048.0;
    
  static size_t buttons[] =      {  PB11  ,  PB10 ,    PB1 ,   PB0, PC14, PC15   };
  static char * button_names[] = { "PB11", "PB10",  "PB1",  "PB0", "PC14", "PC15"  };
    
  uint8_t trigger_states = 0;

  _control_event_source.poll();

  while(
    application_event ae = process_control_event(
      _control_event_source.dequeue_event()
    )
  ) {
    if (ae.type == application_event_type::EVT_TRIGGER) {
      trigger_states |= 1 << ae.parameter;
    }
  }
  
  for (size_t ix = 0; ix < 6; ix++) {
    if (trigger_states & (1 << ix)) {
      Serial.print("Triggered ");
      Serial.print(button_names[ix]);
      
      if ((ix == 2) || (ix == 3)) {
        Serial.print(" ");
      }
      
      Serial.print(" @ 0x");
      Serial.print(_voices[_voices_map[ix]]->amplitude, HEX);
      Serial.print(" / ");
      Serial.println(ix);
      
      _voices[_voices_map[ix]]->trigger  = true;
      _voices[_voices_map[5]]->trigger  &= ! _voices[_voices_map[4]]->trigger;
    }
  }
}


////////////////////////////////////////////////////////////////////////////////
  
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
    sample tmp = _voices[_voices_map[ix]]->play();
      
    if ((ix == 5) && _voices[_voices_map[ix]]->trigger) {
      Serial.println(tmp);
    }
      
    sample_ += tmp;
  }

  sample_    >>= 1;
  sample_     *= _master_vol;
  _avg_sample += sample_;

  _dac.write_mono(sample_);
 
  _sample_ix  ++;
  _sample_ix  %= Samples::NUM_ELEMENTS; 
}

////////////////////////////////////////////////////////////////////////////////

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
  
////////////////////////////////////////////////////////////////////////////////

void application::loop(void) {
  if (_draw_buffer.readable()) {
    graph();
  }
}

////////////////////////////////////////////////////////////////////////////////


/* Local Variables:  */
/* fill-column: 100  */
/* End:              */
