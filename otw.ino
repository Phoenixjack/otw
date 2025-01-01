#include <Arduino.h>
#include "pico/multicore.h"
#include "otw_imu.h"
#include "otw_sdcard.h"
#include "otw_systems.h"

// we're keeping the forward declarations separate from the calling of the instance
// helps with interdependencies
Node thisnode;

void setup() {
  Serial.begin(115200);
  thisnode.set_defaults();
  thisnode.check_minimum();
  multicore_launch_core1(core1_loop);
}

void loop() {
  thisnode.monitor_systems();
  thisnode.retry_faults();
}

void core1_loop() {
  while (true) {
  }
}