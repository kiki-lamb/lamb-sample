#include "controls/controls.h"

//////////////////////////////////////////////////////////////////////////////

controls::signal_configuration          controls::_signals[controls::SIGNALS_COUNT] = {
 { { PA0 }, application_event_type::EVT_UNKNOWN    },
 { { PA1 }, application_event_type::EVT_UNKNOWN    },
 { { PA2 }, application_event_type::EVT_UNKNOWN    },
 { { PA3 }, application_event_type::EVT_FILTER_F_1 },
 { { PA4 }, application_event_type::EVT_FILTER_Q_1 },
 { { PA5 }, application_event_type::EVT_UNKNOWN    },
 { { PA6 }, application_event_type::EVT_PITCH_1    }
};

controls::button          controls::_button_devices[controls::BUTTONS_COUNT] = {
 { PB9,  0 },
 { PB8,  1 },
 { PB7,  2 },
 { PB6,  3 },
 { PA10, 4 },
 { PA9,  5 },
};

controls::combined_source controls::_control_event_source;

//////////////////////////////////////////////////////////////////////////////

void controls::setup() {
 uint8_t ix = 0;
 
 for (size_t bix = 0; bix < BUTTONS_COUNT; ix++, bix++) {
  _button_devices[bix].button_number = bix;
  _button_devices[bix].setup();
  
  _control_event_source.sources[ix] =
   new button_source(&_button_devices[bix]);
 }

 for (size_t six = 0; six < SIGNALS_COUNT; ix++, six++) {
  _signals[six].signal.signal_number = six;
  _signals[six].signal.setup();
  
  _control_event_source.sources[ix] =
   new signal_source(&_signals[six].signal);
 }
}

////////////////////////////////////////////////////////////////////////////////

size_t controls::queue_count() {
 return _control_event_source.queue_count();
}

////////////////////////////////////////////////////////////////////////////////

void controls::poll() {
#ifdef LOG_EVENT_TIMES
 uint32_t start = millis();
#endif
 _control_event_source.poll();
  
#ifdef LOG_EVENT_TIMES
 uint32_t delta = millis() - start;

 Serial.print("Poll took ");
 Serial.print(delta);
 Serial.print(" ms, have ");
 Serial.print(_control_event_source.queue_count());
 Serial.print(" events.");
 Serial.println();
#endif
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
 uint16_t          sig_val = control_event.parameter & 0x0fff;
 uint8_t           sig_num = (control_event.parameter& 0xf000) >> 12;
 application_event application_event(application_event_type::EVT_UNKNOWN, sig_val);

 if (sig_num < SIGNALS_COUNT) {
  application_event.type      = _signals[sig_num].application_event_type;
  application_event.parameter = sig_val;
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
