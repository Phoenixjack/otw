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
  void sync_time();
  void start_pps_sync();
  void pps_sync_ISR();
  void check_and_act();
  void stage_data();
} gps;

bool gps::init() {                // initialize the port and try to configure it
attachInterrupt(digitalPinToInterrupt(gps_pps_pin), pps_sync_ISR, RISING); GPS.begin(9600);                                // usually 4800 or 9600 baud
  GPS.sendCommand(PMTK_SET_NMEA_OUTPUT_RMCGGA);   // may not work with all gps modules
  GPS.sendCommand(PMTK_SET_NMEA_OUTPUT_RMCONLY);  // ditto
  GPS.sendCommand(PMTK_SET_NMEA_UPDATE_1HZ);      // 1 Hz update rate
  unsigned long timeout = millis() + 1500;        // set a listening timeout
  ready = false;    // used in case we need to re-initialize GPS
  while (millis() < timeout && !ready) {          // as long as we haven't timed out AND we haven't recieved a proper GPS sentence
    ready = check();             // keep checking
    start_pps_sync();
  }
  return ready;
};

bool gps::check() {  // TODO: here is where we'll check next_time_sync and then enable interrupts, get PPS, disable interrupts and update the epoch time
  char c = GPS.read();
  last_fix_sec = (uint8_t)GPS.secondsSinceFix();
  newdata = GPS.newNMEAreceived();
  if (last_fix_sec > 10) { newdata = false; } else { stage_data(); }
  return newdata;
};

void gps::sync_time(){
  pps_sync_in_progress
// get PPS 
// set flag
// disable interrupts
  
};

void gps::start_pps_sync() {
  pps_sync_in_progress = true;
  interrupts(); // enable interrupts
};

void gps::pps_sync_ISR() {
  pps_sync_in_progress = false;
  micros_capture = micros();
};

void gps::stage_data() { 
  time_stamp = millis();


  if (GPS.year == 0) {               // it's possible to get NMEA sentences WITHOUT even a timefix yet; if we don't even have that, don't set epoch time
    epoch_time = 0;           // this is Chris telling Chris that Chris has a problem
  } else {                           // I got 99 problems, but this ain't one
    stamp.setDateTime(GPS.year + 2000, GPS.month, GPS.day, GPS.hour, GPS.minute, GPS.seconds);
    epoch_time = stamp.getUnix();
  }
  if (GPS.parse(GPS.lastNMEA())) {  // only load GPS data if we have a GPS fix
    report.gps_lock = GPS.fix;
    report.fixquality = GPS.fixquality;
    if (report.gps_lock) {  // only load GPS data if we have a GPS lock
      report.lat = GPS.latitudeDegrees;
      report.lon = GPS.longitudeDegrees;
      report.altitude = GPS.altitude;
    } else {
      report.lat = 0;
      report.lon = 0;
    }
  }
};