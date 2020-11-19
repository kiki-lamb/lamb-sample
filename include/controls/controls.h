#ifndef BP_DRUM_CONTROLS_H
#define BP_DRUM_CONTROLS_H

//////////////////////////////////////////////////////////////////////////////////////////

#include "lamb.h"
#include "events/control.h"
#include "events/application.h"

//////////////////////////////////////////////////////////////////////////////////////////

class controls {

//////////////////////////////////////////////////////////////////////////////////////////

public:

 /////////////////////////////////////////////////////////////////////////////////////////

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

 /////////////////////////////////////////////////////////////////////////////////////////

 static constexpr uint32_t             K_RATE              = 80;
 static constexpr uint8_t              SIGNALS_COUNT       = 4;
 static constexpr uint8_t              BUTTONS_COUNT       = 6;
 static constexpr uint8_t              EVENT_SOURCES_COUNT = SIGNALS_COUNT + BUTTONS_COUNT;

 /////////////////////////////////////////////////////////////////////////////////////////
 
 typedef lamb::events::sources::combine<
  control_event,
  EVENT_SOURCES_COUNT>                                       combined_source;
 
//////////////////////////////////////////////////////////////////////////////////////////

private:

 /////////////////////////////////////////////////////////////////////////////////////////

 template <typename device_t_, typename source_t_>
 struct configuration {
  typedef device_t_ device_t;
  typedef source_t_ source_t;

  device_t device;
  
  events::application_event_type application_event_type;
 };

  //---------------------------------------------------------------------------------------

 template <uint8_t count, typename device_t_, typename source_t_>
 static void configure(configuration<device_t_, source_t_> arr[count], uint8_t & ix) {
  for (uint8_t bix = 0; bix < count; ix++, bix++) {
   arr[bix].device.number = bix;
   arr[bix].device.setup();
   
   _control_event_source.sources[ix] =
    new source_t_(&arr[bix].device);   
  }
 }

 //---------------------------------------------------------------------------------------

 typedef configuration<signal, signal_source>  signal_configurations[SIGNALS_COUNT];
 typedef configuration<button, button_source>  button_configurations[BUTTONS_COUNT];
 
 /////////////////////////////////////////////////////////////////////////////////////////
 
 static  signal_configurations   _signals;
 static  button_configurations   _buttons;
 static  combined_source         _control_event_source;

 /////////////////////////////////////////////////////////////////////////////////////////

 static       application_event    process_control_event(
  control_event const & control_event
 );
 
 template <control_event_type cet>
 static  application_event process(
  control_event const & control_event
 );

//////////////////////////////////////////////////////////////////////////////////////////
 
public:

 /////////////////////////////////////////////////////////////////////////////////////////

 static       void                 setup();
 static       void                 loop();
 static       void                 poll();
 static       application_event    dequeue_event();
 static       size_t               queue_count();

//////////////////////////////////////////////////////////////////////////////////////////

};

#endif

//////////////////////////////////////////////////////////////////////////////////////////

/* Local Variables:  */
/* fill-column: 100  */
/* End:              */
