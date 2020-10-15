#ifndef BP_DRUM_CONTROL_EVENT_H
#define BP_DRUM_CONTROL_EVENT_H

#include <Arduino.h>
#include "lamb.h"

namespace events {
  enum control_event_type {
    CTL_EVT_NOT_AVAILABLE = 0,       
    EVT_ENCODER       = 1,
    EVT_BUTTON        = 2,
    EVT_SIGNAL        = 4
  };

  class control :
    public lamb::events::event<control_event_type> {
  public:
    control(
      event_type type_ = event_type::CTL_EVT_NOT_AVAILABLE,
      uint16_t arg = 0
    ) :
      event(type_, arg) {}
  };
}

#endif
