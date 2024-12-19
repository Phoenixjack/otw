#include <UnixTime.h>          // https://www.unixtimestamp.com/index.php
UnixTime stamp(0);             // a timestamp object with no time zone offset
#include <Adafruit_GPS.h>      // https://github.com/adafruit/Adafruit_GPS
#define GPSSerial Serial2      // tying Serial2 / TX1/RX1 / hardware pin 11 & 12 to GPS module
Adafruit_GPS GPS(&GPSSerial);  // initiate

struct gps {
  bool ready = false;                  // overall subsystem readiness flag
  bool newdata = false;  // we'll set this anytime the library reports valid sentences and clear it once it's been picked up 
  uint8_t last_fix_sec = 0;
  uint32_t time_stamp = 0;             // millis() timestamp of when the gps was processed
  uint8_t seconds_since_last_fix = 0;  // used to track GPS.secondsSinceFix()
  float lat, lon, altitude;            // gps data
  uint8_t fixquality = 0;              //
  bool lock = false;                   //
  bool pps_sync_in_progress = false; // we'll set this flag while listening 
  uint32_t micros_capture = 0; // we'll store the micros() here in the interrupt routine
  uint32_t epoch_time_sec = 0;             // holds GPS time as epoch timestamp in seconds
  uint32_t epoch_time_usec = 0; // will hold the LSB portion of epoch time
  uint32_t next_time_sync = 0;         // holds the next millis() when we'll conduct a epoch calculation and sync to PPS
  bool init();
  bool check();
  void pps_sync_enable();
  void pps_sync_disable();
  void pps_sync_ISR();
  uint64_t getEpochTime_usec();
  void stage_data();
} gps;

bool gps::init() {                // initialize the port and try to configure it GPS.begin(9600);                                // usually 4800 or 9600 baud
  GPS.sendCommand(PMTK_SET_NMEA_OUTPUT_RMCGGA);   // may not work with all gps modules
  GPS.sendCommand(PMTK_SET_NMEA_OUTPUT_RMCONLY);  // ditto
  GPS.sendCommand(PMTK_SET_NMEA_UPDATE_1HZ);      // 1 Hz update rate
  unsigned long timeout = millis() + 1500;        // set a listening timeout
  ready = false;    // used in case we need to re-initialize GPS
  while (millis() < timeout && !ready) {          // as long as we haven't timed out AND we haven't recieved a proper GPS sentence
    ready = check();             // keep checking
  }
  return ready;
};

bool gps::check() {
  char c = GPS.read();
  last_fix_sec = (uint8_t)GPS.secondsSinceFix();
  newdata = GPS.newNMEAreceived(); // process any data received so far
  if (last_fix_sec > 10) {
    newdata = false; // we've timed out 
    gps_lock = false; // and lost the fix
  } else { 
    stage_data(); 
  }
  if (millis() > next_time_sync) {
    if (pps_sync_in_progress) {
      if (micros_capture != 0) { 
        pps_sync_disable(); // captured, so end it
      }
    } else {
      pps_sync_enable(); // it's time to start listening
    }
  }
  return newdata;
};

void gps::pps_sync_enable() {
  pps_sync_in_progress = true;
  micros_capture = 0; attachInterrupt(digitalPinToInterrupt(GPS_PPS_IN), pps_sync_ISR, RISING);
};

void gps::pps_sync_disable() {
 detachInterrupt(digitalPinToInterrupt (GPS_PPS_IN)); // disable interrupts so it doesn't happen again 
  epoch_time_usec = micros() - micros_capture;
  pps_sync_in_progress = false; // clear the flag
  if (GPS.year > 0) {   // I got 99 problems, but this bitch ain't one
    stamp.setDateTime(GPS.year + 2000, GPS.month, GPS.day, GPS.hour, GPS.minute, GPS.seconds);
    epoch_time = stamp.getUnix();
    if (epoch_time_usec > 1000000) { // if it's been more than a second since we captured pps
      epoch_time += (epoch_time_usec/1000000);
      epoch_time_usec %= 1000000;
    }
  } else {
    epoch_time = 0; // it's invalid, so ditch the value
    epoch_time_usec = 0; // ditto
  }
  next_time_sync = millis() + 120000; 
};

void gps::pps_sync_ISR() {
  if (pps_sync_in_progress) { // just in case the ISR gets called again before postprocessing is complete
    micros_capture = micros();
    pps_sync_in_progress = false;
  }
};

uint64_t gps::getEpochTime_usec() { 
  if (epoch_time > 0) {
    uint64_t time_value = (uint64_t)epoch_time * 1000000; // leave room for msec and usec
    if (epoch_time_usec > 0) {
      // time_value += (uint64_t)
    }
  } else {
    return 0;
  } 
};
void gps::stage_data() { 
  time_stamp = millis();
  if (GPS.parse(GPS.lastNMEA())) {  // only load GPS data if we have a GPS fix
    gps_lock = GPS.fix;
    fixquality = GPS.fixquality;
    if (gps_lock) {  // only load GPS data if we have a GPS lock
      lat = GPS.latitudeDegrees;
      lon = GPS.longitudeDegrees;
      altitude = GPS.altitude;
    } else {
      lat = 0;
      lon = 0;
    }
  }
};