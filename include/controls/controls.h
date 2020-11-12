#ifndef BP_DRUM_CONTROLS_H
#define BP_DRUM_CONTROLS_H

#include "lamb.h"
#include "events/control.h"
#include "events/application.h"

class controls {
public:
 typedef lamb::controls::button                              button;
 typedef lamb::controls::analog                              signal;
 typedef events::control                                     control_event;
 typedef events::control_event_type                          control_event_type;
 typedef events::application                                 application_event;
 typedef events::application_event_type                      application_event_type;
  
 typedef lamb::events::sources::analog<
  signal,
  control_event,
  control_event_type::EVT_SIGNAL
  >                                                          signal_source;

 typedef lamb::events::sources::buttons<
  button,
  control_event,
  control_event_type::EVT_BUTTON
  >                                                          button_source;

 static constexpr uint8_t              SIGNALS_COUNT       = 7;
 static constexpr uint8_t              BUTTONS_COUNT       = 6;
 static constexpr uint8_t              EVENT_SOURCES_COUNT = SIGNALS_COUNT + BUTTONS_COUNT;

 typedef lamb::events::sources::combine<
  control_event,
  EVENT_SOURCES_COUNT>                                       combined_source;

private:

 struct signal_configuration {
  controls::signal               signal;
  events::application_event_type application_event_type;
 };
 
 static signal_configuration           _signals[SIGNALS_COUNT];
 static button                         _button_devices[BUTTONS_COUNT]; 
 static combined_source                _control_event_source;

 static       application_event    process_control_event(
  control_event const & control_event
 );

 static       application_event    process_button_event(
  control_event const & control_event
 );

 static       application_event    process_encoder_event(
  control_event const & control_event
 );

 static       application_event    process_signal_event(
  control_event const & control_event
 );
  
public:
 static       void                 setup();
 static       void                 loop();
 static       void                 poll();
 static       application_event    dequeue_event();
 static       size_t               queue_count();
};

#endif

////////////////////////////////////////////////////////////////////////////////

/* Local Variables:  */
/* fill-column: 100  */
/* End:              */
