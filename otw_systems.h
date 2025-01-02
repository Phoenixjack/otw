#include <functional>
#include <array>
#include <cstdint>

/* System usage: snprintf(system[i].error_plain_text, sizeof(system[i].error_plain_text), "Error: %d", error_code);
   System default: strncpy(error_plain_text, "No Error", sizeof(error_plain_text));
   const char* ERROR_MESSAGES[] = {
    "No Error",
    "SD Init Failed",
    "SD Health Error",
    "Sensor Disconnected",
    "Unknown Error"
  };
  Node.systems[sys_sd].error_plain_text = ERROR_MESSAGES[Node.systems[sys_sd].error_code];
*/

enum SYSTEM_NAMES {  // Define your system names
  sys_bn55,
  sys_sd,
  sys_gps,
  sys_lcd,
  sys_lora,
  sys_wifi,
  sys_end_of_list
};

enum SYSTEM_STATE {  // Define system states
  STATE_UNINITIALIZED,
  STATE_READY,
  STATE_FAULT
};

struct System {  // System structure
  SYSTEM_STATE state = STATE_UNINITIALIZED;
  bool required = false;
  uint32_t last_valid_time = 0;  // Last valid check time
  std::function<bool()> init;    // Initialization function
  std::function<bool()> check;   // Periodic health check function
  uint8_t error_code = 0;        // Last error code
  char error_plain_text[21];     // for simplicity, we'll allow each subsystem to return a 20 character easily readable error status
};

struct Node {  // Node structure
  std::array<System, sys_end_of_list> systems;
  void set_defaults() {
    systems[sys_bn55].required = true;
    systems[sys_bn55].init = []() {
      return bno055.init();
    };
    systems[sys_bn55].check = []() {
      return bno055.check();
    };

    systems[sys_sd].required = true;
    systems[sys_sd].init = []() {  // <<<<<<<<<<<<<<<<<<<<<<<<< add code here for if sd good, then datalogger.init();
      return sd.init();
    };
    systems[sys_sd].check = []() {
      return true;  // sd.check();
    };

    systems[sys_gps].init = []() {
      return true;  // gps.init();
    };
    systems[sys_gps].check = []() {
      return true;  // gps.check();
    };

    systems[sys_lcd].init = []() {
      return true;  // lcd.init();
    };
    systems[sys_lcd].check = []() {
      return true;  // lcd.check();
    };
    // Add other systems similarly
  }
  void check_minimum() {
    for (auto& system : systems) {
      if (system.required && system.state != STATE_READY) {
        if (system.init && system.init()) {
          system.state = STATE_READY;
          system.last_valid_time = millis();
        } else {
          system.state = STATE_FAULT;
        }
      }
    }
  }

  void retry_faults() {
    uint32_t current_time = millis();
    for (auto& system : systems) {
      if (system.state == STATE_FAULT) {
        if (system.init && system.init()) {
          system.state = STATE_READY;
          system.last_valid_time = current_time;
        }
      }
    }
  }

  void monitor_systems() {
    uint32_t current_time = millis();
    for (auto& system : systems) {
      if (system.check && system.state == STATE_READY) {
        if (!system.check()) {
          system.state = STATE_FAULT;
        } else {
          system.last_valid_time = current_time;
        }
      }
    }
  }
};
