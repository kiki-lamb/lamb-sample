#include "controls/controls.h"

//////////////////////////////////////////////////////////////////////////////

controls::signal          controls::_signal_device0    ( PA3,  3                   );
controls::signal          controls::_signal_device1    ( PA4,  4                   );
controls::signal          controls::_signal_device2    ( PA6,  6                   );
controls::button          controls::_button_device0    ( PB9,  0                   );
controls::button          controls::_button_device1    ( PB8,  1                   );
controls::button          controls::_button_device2    ( PB7,  2                   );
controls::button          controls::_button_device3    ( PB6,  3                   );
controls::button          controls::_button_device4    ( PA10, 4                   );
controls::button          controls::_button_device5    ( PA9,  5                   );
controls::control_source  controls::_control_event_source;

//////////////////////////////////////////////////////////////////////////////

void controls::setup() {
  _signal_device0      .setup();
  _signal_device1      .setup();
  _signal_device2      .setup();
      
  _button_device0      .setup();
  _button_device1      .setup();
  _button_device2      .setup();
  _button_device3      .setup();
  _button_device4      .setup();
  _button_device5      .setup();

  combined_source * cs         = new combined_source;
  cs->sources[0]               = new signal_source(&_signal_device0);
  cs->sources[1]               = new signal_source(&_signal_device1);
  cs->sources[2]               = new signal_source(&_signal_device2);
  cs->sources[3]               = new button_source(&_button_device0);
  cs->sources[4]               = new button_source(&_button_device1);
  cs->sources[5]               = new button_source(&_button_device2);
  cs->sources[6]               = new button_source(&_button_device3);
  cs->sources[7]               = new button_source(&_button_device4);
  cs->sources[8]               = new button_source(&_button_device5);
  _control_event_source.source = cs;
}

////////////////////////////////////////////////////////////////////////////////

void controls::poll() {
  _control_event_source.poll();
}

////////////////////////////////////////////////////////////////////////////////

controls::application_event controls::dequeue_event() {
  return process_control_event(
    _control_event_source.dequeue_event()
  );
}

////////////////////////////////////////////////////////////////////////////////

controls::application_event controls::process_control_event(
  controls::control_event const & control_event
) {
  application_event application_event;
  application_event.type = application_event_type::EVT_UNKNOWN;

  switch (control_event.type) {
  case control_event_type::CTL_EVT_NOT_AVAILABLE:
    application_event.type = application_event_type::APP_EVT_NOT_AVAILABLE;    
    return application_event;
    
  case control_event_type::EVT_SIGNAL:
    return process_signal_event(control_event);

  case control_event_type::EVT_BUTTON:
    return process_button_event(control_event);  

  case control_event_type::EVT_ENCODER:
    return process_encoder_event(control_event);  
  }
  
  return application_event;
}

////////////////////////////////////////////////////////////////////////////////

controls::application_event controls::process_signal_event(
  controls::control_event const & control_event
) {
  uint16_t sig_val = control_event.parameter & 0x0fff;
  uint8_t  sig_num = (control_event.parameter& 0xf000) >> 12;

  application_event application_event;

  if (sig_num == 6) {
    application_event.type           = application_event_type::EVT_PITCH_1;
    application_event.parameter      = sig_val;
  }
  else if (sig_num == 3) {
    application_event.type           = application_event_type::EVT_FILTER_F_1;
    application_event.parameter      = sig_val;
  }
  else if (sig_num == 4) {
   application_event.type           = application_event_type::EVT_FILTER_Q_1;
    application_event.parameter      = sig_val;
  }
  else {
    application_event.type           = application_event_type::APP_EVT_NOT_AVAILABLE;
  }

#ifdef LOG_SIGNALS
  Serial.print(F("Signal "));
  Serial.print(sig_num);
  Serial.print(F(" = "));
  Serial.print (sig_val);
  Serial.println();
#endif

  return application_event;
}

////////////////////////////////////////////////////////////////////////////////

controls::application_event controls::process_encoder_event(
  controls::control_event const & control_event
) {
  application_event application_event;
  application_event.type           = application_event_type::EVT_UNKNOWN;

  Serial.println("Don't know how to process encoders yet!");

  return application_event;
}

////////////////////////////////////////////////////////////////////////////////

controls::application_event controls::process_button_event(
  controls::control_event const & control_event
) {
  application_event application_event;
  application_event.type           = application_event_type::EVT_UNKNOWN;
  uint8_t           button_number  = control_event.parameter_hi();

#ifdef LOG_BUTTONS
  int8_t            button_state   = (int8_t)control_event.parameter_lo(); 

  Serial.print(F("Button event, number: "));
  Serial.print(button_number);
  Serial.print(F(", state: "));
  Serial.print(button_state);
  Serial.println();
#endif

  application_event.type      = application_event_type::EVT_TRIGGER;
  application_event.parameter = button_number;
  
  return application_event;
}

////////////////////////////////////////////////////////////////////////////////

/* Local Variables:  */
/* fill-column: 100  */
/* End:              */
