#ifndef BP_DRUM_CONTROLS_H
#define BP_DRUM_CONTROLS_H

#include "lamb.h"
#include "events/control.h"
#include "events/application.h"

class controls {
public:
  typedef lamb::controls::button                           button;
  typedef lamb::controls::analog                           signal;
  typedef events::control                                  control_event;
  typedef events::control_event_type                       control_event_type;
  typedef events::application                              application_event;
  typedef events::application_event_type                   application_event_type;
  typedef lamb::events::sources::buffer<control_event, 16> control_source;
  
  typedef lamb::events::sources::analog<
    signal,
    control_event,
    control_event_type::EVT_SIGNAL
    >                                                      signal_source;

  typedef lamb::events::sources::buttons<
    button,
    control_event,
    control_event_type::EVT_BUTTON
    >                                                      button_source;
  
  static const uint8_t              EVENT_SOURCES_COUNT = 9;

  typedef lamb::events::sources::combine<
    control_event,
    EVENT_SOURCES_COUNT>                                   combined_source;

private:
  static       signal               _signal_device0;
  static       signal               _signal_device1;
  static       signal               _signal_device2;
  
  static       signal_source        _signal_source0;
  static       signal_source        _signal_source1;
  static       signal_source        _signal_source2;
  
  static       button               _button_device0;
  static       button               _button_device1;
  static       button               _button_device2;
  static       button               _button_device3;
  static       button               _button_device4;
  static       button               _button_device5;
  
  static       button_source        _button_source0;
  static       button_source        _button_source1;
  static       button_source        _button_source2;
  static       button_source        _button_source3;
  static       button_source        _button_source4;
  static       button_source        _button_source5;
  
  static       combined_source      _combined_source;
  
  static       control_source       _control_event_source;
  
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
};

#endif

/* Local Variables:  */
/* fill-column: 100  */
/* End:              */
