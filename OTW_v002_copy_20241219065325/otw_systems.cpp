struct thisnode {
  enum SYSTEM_NAMES {  //
    sys_gps,
    sys_bn55,
    sys_sd,
    sys_lora,
    sys_lcd,
    sys_wifi,
    sys_dataIn,
    sys_end_of_list
  };
  struct system {
    bool ready = false;            // has it been initialized?
    bool fault = false;            // is the system in a fault state?
    uint32_t time_last_valid = 0;  // last time the system was verified good
  } system[sys_end_of_list];
  uint32_t epoch_boot_time_sec = 0;   // duh
  uint32_t epoch_boot_time_msec = 0;  // JUST the milliseconds/microseconds offset from epoch seconds
  uint32_t next_epoch_sync_time = 0;  // the minimum millis before we attempt another GPS/epoch sync
  void set_defaults();
  bool minimum_required_to_boot();    // include any boolean flags that we ABSOLUTELY must have to function
} thisnode;

void thisnode::set_defaults(){
  
};
bool thisnode::minimum_required_to_boot() {  // include any boolean flags that we ABSOLUTELY must have to function
  return (system[sys_bn55].ready & system[sys_sd].ready);
};
