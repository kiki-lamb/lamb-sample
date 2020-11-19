#include "controls/controls.h"

//////////////////////////////////////////////////////////////////////////////

controls::signal_configuration controls::_signals[controls::SIGNALS_COUNT] = {
 // { { PA0    }, application_event_type::EVT_UNKNOWN    },
 // { { PA1    }, application_event_type::EVT_UNKNOWN    },
 // { { PA2    }, application_event_type::EVT_UNKNOWN    },
 { { PA3, 3 }, application_event_type::EVT_FILTER_F_1 },
 { { PA4, 3 }, application_event_type::EVT_FILTER_Q_1 },
 { { PA5, 3 }, application_event_type::EVT_VOLUME     },
 { { PA6, 3 }, application_event_type::EVT_PITCH_1    },
};

controls::button_configuration  controls::_buttons[controls::BUTTONS_COUNT] = {
 { { PB9  }, application_event_type::EVT_TRIGGER   },
 { { PB8  }, application_event_type::EVT_TRIGGER   },
 { { PB7  }, application_event_type::EVT_TRIGGER   },
 { { PB6  }, application_event_type::EVT_TRIGGER   },
 { { PA10 }, application_event_type::EVT_TRIGGER   },
 { { PA9  }, application_event_type::EVT_TRIGGER   },
};

controls::combined_source controls::_control_event_source;

//////////////////////////////////////////////////////////////////////////////

template <typename s_t, uint8_t count, typename c_t>
void controls::setup(c_t arr[count], uint8_t & ix) {
 for (uint8_t bix = 0; bix < count; ix++, bix++) {
  arr[bix].device.number = bix;
  arr[bix].device.setup();

  _control_event_source.sources[ix] =
   new s_t(&arr[bix].device);   
 } 
}

//////////////////////////////////////////////////////////////////////////////

void controls::setup() {
 uint8_t ix = 0;

 setup<button_source, BUTTONS_COUNT>(_buttons, ix);
 setup<signal_source, SIGNALS_COUNT>(_signals, ix);
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

template <>
controls::application_event controls::process<controls::control_event_type::EVT_SIGNAL>(
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

template <>
controls::application_event controls::process<controls::control_event_type::EVT_BUTTON>(
 controls::control_event const & control_event
) {
 uint8_t           button_number  = control_event.parameter_hi();
 application_event application_event(application_event_type::EVT_UNKNOWN, button_number);

 if (button_number < BUTTONS_COUNT) {
  application_event.type      = _buttons[button_number].application_event_type;
 } 

#ifdef LOG_BUTTONS
 Serial.print(F("Button event, number: "));
 Serial.print(button_number);
 Serial.println();
#endif
  
 return application_event;
}

////////////////////////////////////////////////////////////////////////////////

template <>
controls::application_event controls::process<controls::control_event_type::EVT_ENCODER>(
 controls::control_event const & control_event
) {
 application_event application_event(application_event_type::EVT_UNKNOWN, 0);

 Serial.println("Don't know how to process encoders yet!");

 return application_event;
}

////////////////////////////////////////////////////////////////////////////////

controls::application_event controls::process_control_event(
 controls::control_event const & control_event
) {
 switch (control_event.type) {
 case control_event_type::CTL_EVT_NOT_AVAILABLE:
  return application_event(application_event_type::APP_EVT_NOT_AVAILABLE);
  
 case control_event_type::EVT_SIGNAL:
  return process<control_event_type::EVT_SIGNAL>(control_event);

 case control_event_type::EVT_BUTTON:
  return process<control_event_type::EVT_BUTTON>(control_event);  

 case control_event_type::EVT_ENCODER:
  return process<control_event_type::EVT_ENCODER>(control_event);  
 
 default: 
  return application_event(application_event_type::EVT_UNKNOWN);
 }
}
 
////////////////////////////////////////////////////////////////////////////////

/* Local Variables:  */
/* fill-column: 100  */
/* End:              */
