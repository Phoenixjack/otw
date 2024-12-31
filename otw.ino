#include <Arduino.h>
#include "pico/multicore.h"
#include "otw_systems.h"


void setup() {
  Serial.begin(115200);
  multicore_launch_core1(core1_loop);
}

void loop() {

}

void core1_loop() {
  while (true) {


  }
}