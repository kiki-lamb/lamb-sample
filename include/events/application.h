#ifndef BP_DRUM_APPLICATION_EVENT_H
#define BP_DRUM_APPLICATION_EVENT_H

#include <Arduino.h>
#include "lamb.h"

namespace events {
  enum application_event_type {
    APP_EVT_NOT_AVAILABLE = 0,

    EVT_TRIGGER  = 1,
    EVT_VOLUME   = 2,
    EVT_PITCH_1  = 3,
    EVT_PITCH_2  = 4,
    EVT_PITCH_3  = 5,
    EVT_FILTER_F_1 = 6,
    EVT_FILTER_Q_1 = 7,
    EVT_UNKNOWN  = 64
  };  

  class application :
    public lamb::events::event<application_event_type> {
  public:
    application(
      event_type type_ = event_type::APP_EVT_NOT_AVAILABLE,
      uint16_t arg = 0
    ) :
      event(type_, arg) {}
  };
}

#endif
