struct thisnode {
  enum SYSTEM_NAMES {  //
    sys_bn55,
    sys_sd,
    sys_gps,
    sys_lora,
    sys_lcd,
    sys_wifi,
    sys_dataIn,
    sys_end_of_list
  };
  enum OUTPUT_TYPES {  //
    output_serial_monitor,
    output_sd,
    output_lora,
    output_wifi,
    output_lcd,
    output_end_of_list
  };

  struct outputs {
    bool enabled = true;
    uint32_t interval_msec = 1000;
    uint32_t next_update = 0;
  } outputs[output_end_of_list];

  struct system {
    bool ready = false;            // has it been initialized?
    bool required= false; // is this system required to run?
    bool fault = false;            // is the system in a fault state?
    uint32_t time_last_valid = 0;  // last time the system was verified good
    bool *init = *nullptr; // points to the init routine for each system 
    uint8_t error_code 0; // holds the error code for each system 
    bool *check() = *nullptr; // points to the query routine for each system 
  } system[sys_end_of_list];
  uint32_t epoch_boot_time_sec = 0;   // duh
  uint32_t epoch_boot_time_msec = 0;  // JUST the milliseconds/microseconds offset from epoch seconds
  uint32_t next_epoch_sync_time = 0;  // the minimum millis before we attempt another GPS/epoch sync
  void set_defaults();
  void minimum_not_met();
  void retry();
} thisnode;

void thisnode::set_defaults(){
    system[sys_bn55].required = true;
    system[sys_bn55].init = &bn55.init();
    system[sys_bn55].check = &bn55.check();
    system[sys_sd].required = true;
    system[sys_sd].init = &sd.init();
    system[sys_sd].check = &sd.check();

    system[sys_gps].init = &gps.init();
    system[sys_lora].init = &lora.init();
    system[sys_lcd].init = &lcd.init();
    // .... point to other functions...

    outputs[output_sd].enabled = true;
    outputs[output_sd].interval = 1000;
    outputs[output_lcd].enabled = true;
    outputs[output_lcd].interval = 100;


};

void thisnode::minimum_not_met(){
  for (uint8_t i=0;i<sys_end_of_list;i++){
      if (system[i].required && !system[i].ready) {
        if (system[i].init()) {
           system[i].ready = true;
           time_last_valid = millis();
        }
      }
   }
};

void thisnode::retry() { // similar to init, but reinits with nonblocking delays
    // make use of system[i].ready, fault, & time_last_valid

};