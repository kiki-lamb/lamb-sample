#ifndef APPLICATION_H
#define APPLICATION_H

#include <inttypes.h>
#include "tracks.h"
#include "samples.h"
#include "lamb.h"

namespace Application {
  void lrate();
  void clock();
  void krate();
  void srate();
  void graph();  
  void draw_text();
  void setup();
  void loop();
}
#endif
