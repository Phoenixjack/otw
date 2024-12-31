#define def_column_labels "msec_uptime,epoch_sec,heading,lat,lon,packet_num,gid,uid,tempC,vcc,dut_rssi,dut_noise,stick_rssi,stick_noise,gps"

char printbuffer[256];  // debugging / remove later

#include "otw_systems.cpp"
#include <PhoenixJack_serialbuffer_V0.h>
PhoenixJack_serialbuffer SerMon(&Serial);   // Serial Monitor
PhoenixJack_serialbuffer MiniDL(&Serial1);  // Pin 2 = RX0 // 115200
//#include "otw_report.cpp"
//#include "otw_packetstats.cpp"
//#include "otw_gpio.cpp"
//#include "otw_imu.cpp"
//#include "otw_gps.cpp"
//#include "PhoenixJack_serializeddatalogger_V2.h"




#include "bn0055_gpio.h"
#include "bn0055_report.h"
#include "bn0055_sensor.h"
#include "bn0055_gps.h"
#include "modified_sd.h"
//#include "PhoenixJack_lora_v0.h"
// WIFI: the end goal is https://gitlab.com/painlessMesh/painlessMesh
#include "bn0055_packetstats.h"

struct subsystems {
  bool gps = false;
  bool bn55 = false;
  bool sdcard = false;
  bool lcd = false;
  bool wifi = false;
  bool mqtt = false;
  bool lora = false;
  bool minidl = false;  // goes true if we've received packets; false if we fail to receive a packet after so long
  bool minimum_required_to_boot() {
    return (bn55 & sdcard);  // include any boolean flags that we ABSOLUTELY must have to function
  };
} subsystems;

void setup() {
  initialize_uarts();                       // bootmessage is wordy, so I'm simplifying
  gpio.init();                              // start up the indicator LEDs and inputs
  thisnode.sys_bn55.ready = bn0055.init();  // initialize the sensor
  initialize_sdcard();                      // contains a lot of error checking and user feedback
  thisnode.sys_sd.ready = gps.init();       // gps.init listens for 1.5 seconds; if any valid GPS sentence is received during that time, it returns true
  //thisnode.sys_lcd.ready=true;
  //thisnode.sys_lora.ready=true;
  

  if (subsystems.minimum_required_to_boot()) {  // aggregate the required systems
    onboardled.toggle_blocking(5, 250, 250);    // 5 long flashes before continuing to let the user we're good
    Serial.println("MINIMUM BOOT REQUIREMENTS MET");
  } else {
    LED_Fault.toggle_nonblocking(200, 400);  // continuous flashing to get someone's attention
    Serial.println("MINIMUM BOOT REQUIREMENTS NOT MET");
    while (true) { LED_Fault.check(); }
  }
  gpio.set_runtime_states();
}

void loop() {
  //SerMon.check();                               // CURRENTLY WE DON'T DO ANYTHING WITH THE SERIAL MONITOR INPUT

  gpio_update();

  bn0055_update();

  gps_update();

  minidl_update();

  //report_update(); // DON'T ENABLE until we have NodeRed ready to receive it
}

void initialize_uarts() {
  String bootmsg = "OTW Test\nCOMPILED: ";
  bootmsg += __DATE__;
  bootmsg += " ";
  bootmsg += __TIME__;
  bootmsg += "\n";
  SerMon.init(9600, bootmsg);
  MiniDL.init(115200);
}
void initialize_sdcard() {
  subsystems.sdcard = sd.init();  // initialize the sd card
  if (subsystems.sdcard) {
    sd.start_new_session(def_column_labels);       // start an SD session ID (random 0x0000 to 0xFFFF), and preload our CSV labels that will go in every file
    Serial.print("SD CARD: READY; Session ID: ");  // for debugging; remove later
    Serial.print(sd.get_session_ID(), HEX);        // for debugging; remove later
    Serial.print("; using column labels: ");       // for debugging; remove later
    Serial.println(def_column_labels);             // for debugging; remove later
  } else {
    Serial.print("SD CARD: FAILURE CODE: ");
    Serial.print(sd.current_status);
    switch (sd.current_status) {
      case sd.SD_CANNOT_INITIALIZE:
        Serial.println(" CANNOT INITIALIZE");
        break;
      case sd.SD_NOT_FORMATTED:
        Serial.println(" NOT FORMATTED");
        break;
      case sd.SD_FULL:
        Serial.println(" SD CARD FULL");
        break;
      case sd.SD_COULD_NOT_OPEN_DATA_FILE:
        Serial.println(" COULD NOT OPEN DATA FILE");
        break;
      case sd.SD_COULD_NOT_CREATE_NEW_DATA_FILE:
        Serial.println(" COULD NOT CREATE NEW DATA FILE");
        break;
      case sd.SD_UNABLE_OPEN_FOR_WRITE:
        Serial.println(" UNABLE OPEN FOR WRITE");
        break;
      default:
        Serial.println(" Unknown error. Contact user support.");
    }
  }
}












void gpio_update() {
  gpio.check();                             // query any input buttons/switches and update LEDs as needed
  report.file_save = !SW_FILESAVE.check();  // interrogates the switch setting every XXXmsec and applies that to a flag; we only get FALSE if there's a ground, so we invert the logic
}
void bn0055_update() {
  if (bn0055.ready) {
    if (bn0055.check()) {              // query the sensor
      report.curr_heading = bn0055.x;  // copy the heading to the report struct; good for deconflicting dependencies
      report.heading_timestamp = millis();
    }
  } else {                                                            // if we're here, then the sensor is reporting NOT READY
    if (millis() > bn0055.next_read) {                                // we're probably locked up; reboot the sensor
      Serial.print("BN0055 attempting soft restart; last reading:");  // for debugging; remove later
      Serial.println(bn0055.x);                                       // for debugging; remove later
      BN55_Power.set_off();                                           // power cycle
      BN55_Power.set_on();                                            // power cycle
      bn0055.init();                                                  // reinitialize
      if (bn0055.ready) {                                             // for debugging; remove later
        Serial.println("BN0055 reboot success");                      // for debugging; remove later
      } else {                                                        // for debugging; remove later
        Serial.println("BN0055 reboot failed");                       // for debugging; remove later
      }
    }
  }
}

void gps_update() {
  gps.check_and_act();  // checks GPS and acts on it at the same time; REMEMBER: this loads data to the report struct
}

void minidl_update() {
  MiniDL.check();                                 // query the MiniDL UART port
  if (MiniDL.is_Buffer_Ready_To_Send()) {         // so we'll only attempt to write to file when we have new MiniDL Data Packet
    strcpy(report.packet_buffer, MiniDL.buffer);  // store it in the report buffer for later usage; it's an extra step, but it'll free the receive buffer
    MiniDL.reset();                               // flush it just in case, and reopen the receive buffer for new data
    updatePacketStats();
    updateSD();
  }
}

void updatePacketStats() {
  packet_stats.process_new_msg(MiniDL.get_start_time());  // keep track of stats
  //Serial.print("packet stats avgint: ");                  // for debugging; remove later
  //Serial.println(packet_stats.avg_interval_msec);         // for debugging; remove later
  if (packet_stats.missed_packets_consecutive > 5) {                      // if we've missed more than 5 packets in a row, we have a problem
    Serial.println("missed packets >5; fault light should be blinking");  // for debugging; remove later
    LED_Fault.toggle_nonblocking();
  } else {
    LED_Fault.set_off();
  }
}
void updateSD() {
  subsystems.sdcard = sd.isReady();              // check our current status
  if (subsystems.sdcard) {                       // only if it's good do we attempt to write;
    if (report.file_save) { saveToFile(); }      //
    if (!report.time_set) { updateTimeFile(); }  // IFF the time hasn't been set before, try...
  }
}

void saveToFile() {
  String dataToSave = report.get_CSV_data();  // debug remove
  Serial.print("WRITING TO [" + sd.get_curr_file_name() + "] : ");
  if (sd.write(dataToSave)) {
    Serial.println(dataToSave);
    LED_FWri.single_flash_nonblocking(200);  // turn the LED on for xxx msec IFF we were able to successfully write to file
  } else {
    Serial.print("FAILED; Error Code: ");
    Serial.println(sd.current_status);
  }
}


void updateTimeFile() {
  Serial.println("Attempting to update timefile");  // for debugging; remove later
  if (report.epoch_time > 0) {                      // IFF the time hasn't been set before AND we have a GPS referenced epoch time
    Serial.println("we have a valid epoch time");   // for debugging; remove later
    if (sd.mark_the_time(report.epoch_time)) {      // attempt to write the time to file
      report.time_set = true;                       // if it worked, then set the flag so we don't try again
      Serial.println("timefile updated");           // for debugging; remove later
    }
  } else {
    Serial.println("we don't have a valid epoch time");  // for debugging; remove later
  }
}
void report_update() {                              // if auto-report is enabled AND it's time to send; used to throttle wifi/lora packets
  if (report.can_send()) { report.send_packet(); }  // send it
}
