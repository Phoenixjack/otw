#include <stdio.h>
#include <time.h>
#include <TinyGPSPlus.h>
TinyGPSPlus GPS;
#define GPSSerial Serial2      // tying Serial2 / TX1/RX1 / hardware pin 11 & 12 to GPS module



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
  uint32_t millis_capture = 0; // we'll store the millis() here in the interrupt routine
  uint32_t micros_capture = 0; // we'll store the micros() here in the interrupt routine
  uint32_t epoch_time_sec = 0;             // holds GPS time as epoch timestamp in seconds
  uint64_t up_time_in_usec(uint32_t msec, uint32_t usec); //
  uint64_t epoch_boot_time = 0; // sub millisecond accurate boottime
  uint32_t next_time_sync = 0;         // holds the next millis() when we'll conduct a epoch calculation and sync to PPS
  bool init();
  bool check();
  void pps_sync_enable();
  void pps_sync_disable();
  void pps_sync_ISR();
  uint64_t getEpochTime_usec();
  void stage_data();
  uint32_t getEpoch_sec(uint8_t year2digit, uint8_t month, uint8_t day, uint8_t hour, uint8_t minute, uint8_t  second);
} gps;

bool gps::init() {                // initialize the port and try to configure it GPS.begin(9600);                                // usually 4800 or 9600 baud
  unsigned long timeout = millis() + 1500;        // set a listening timeout
  ready = false;    // used in case we need to re-initialize GPS
  while (millis() < timeout && !ready) {          // as long as we haven't timed out AND we haven't recieved a proper GPS sentence
    ready = check();             // keep checking
  }
  return ready;
};

bool gps::check() {
  GPS.encode(GPSSerial.read());
  newdata = GPS.location.isUpdated();
  

   // GPS.date.isUpdated()

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
  millis_capture = 0;
  micros_capture = 0; attachInterrupt(digitalPinToInterrupt(GPS_PPS_IN), pps_sync_ISR, RISING);
};

uint64_t gps::up_time_in_usec(uint32_t msec = millis(), uint32_t usec = micros()) {
  return ((uint64_t)msec * 1000) + ((uint64_t)(usec%1000));
};

void gps::pps_sync_disable() {
 detachInterrupt(digitalPinToInterrupt (GPS_PPS_IN)); // disable interrupts so it doesn't happen again 
  pps_sync_in_progress = false; // clear the flag
  if (GPS.year > 0) {   // I got 99 problems, but this bitch ain't one
    // stamp.setDateTime(GPS.year + 2000, GPS.month, GPS.day, GPS.hour, GPS.minute, GPS.seconds);
    // epoch_time = stamp.getUnix();

    epoch_time = getEpoch_sec(GPS.year, GPS.month, GPS.day, GPS.hour, GPS.minute, GPS.seconds);
    uint64_t usec_up_time_at_pps = up_time_in_usec(millis_capture, micros_capture);
    epoch_boot_time = (uint64_t)epoch_time * 1000000;
    epoch_boot_time -= usec_up_time_at_pps;
  } else {
    epoch_time = 0; // it's invalid, so ditch the value
    epoch_time_usec = 0; // ditto
  }
  next_time_sync = millis() + 120000; 
};

void gps::pps_sync_ISR() {
  if (pps_sync_in_progress) { // just in case the ISR gets called again before postprocessing is complete
    micros_capture = micros();
    millis_capture = millis();
    pps_sync_in_progress = false;
  }
};

uint64_t gps::getEpochTime_usec() {
  if (epoch_boot_time > 0) {
    return (epoch_boot_time + up_time_in_usec();
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

uint32_t gps::getEpoch_sec(uint8_t year2digit, uint8_t month, uint8_t day, uint8_t hour, uint8_t minute, uint8_t  second) {
    struct tm t;
    time_t t_of_day;
    t.tm_year = year2digit + 100; // referenced from 1900
    t.tm_mon = month - 1; // Month, where 0 = jan
    t.tm_mday = day;          // Day of the month
    t.tm_hour = hour;
    t.tm_min = minute;
    t.tm_sec = second;
    t.tm_isdst = 0;        // Is DST on? 1 = yes, 0 = no, -1 = unknown
    t_of_day = mktime(&t);
    return (uint32_t)t_of_day;
};



/* 
  https://www.epochconverter.com/programming/c
  epoch to human time 
    time_t     now;
    struct tm  ts;
    char       buf[80];

    // Get current time
    time(&now);

    // Format time, "ddd yyyy-mm-dd hh:mm:ss zzz"
    ts = *localtime(&now);
    strftime(buf, sizeof(buf), "%a %Y-%m-%d %H:%M:%S %Z", &ts);
    printf("%s\n", buf);
*/

/*


// For stats that happen every 5 seconds
unsigned long last = 0UL;



void loop()
{
  // Dispatch incoming characters
  while (ss.available() > 0)
    gps.encode(ss.read());

  if (gps.location.isUpdated())
  {
    Serial.print(F("LOCATION   Fix Age="));
    Serial.print(gps.location.age());
    Serial.print(F("ms Raw Lat="));
    Serial.print(gps.location.rawLat().negative ? "-" : "+");
    Serial.print(gps.location.rawLat().deg);
    Serial.print("[+");
    Serial.print(gps.location.rawLat().billionths);
    Serial.print(F(" billionths],  Raw Long="));
    Serial.print(gps.location.rawLng().negative ? "-" : "+");
    Serial.print(gps.location.rawLng().deg);
    Serial.print("[+");
    Serial.print(gps.location.rawLng().billionths);
    Serial.print(F(" billionths],  Lat="));
    Serial.print(gps.location.lat(), 6);
    Serial.print(F(" Long="));
    Serial.println(gps.location.lng(), 6);
  }

  else if (gps.date.isUpdated())
  {
    Serial.print(F("DATE       Fix Age="));
    Serial.print(gps.date.age());
    Serial.print(F("ms Raw="));
    Serial.print(gps.date.value());
    Serial.print(F(" Year="));
    Serial.print(gps.date.year());
    Serial.print(F(" Month="));
    Serial.print(gps.date.month());
    Serial.print(F(" Day="));
    Serial.println(gps.date.day());
  }

  else if (gps.time.isUpdated())
  {
    Serial.print(F("TIME       Fix Age="));
    Serial.print(gps.time.age());
    Serial.print(F("ms Raw="));
    Serial.print(gps.time.value());
    Serial.print(F(" Hour="));
    Serial.print(gps.time.hour());
    Serial.print(F(" Minute="));
    Serial.print(gps.time.minute());
    Serial.print(F(" Second="));
    Serial.print(gps.time.second());
    Serial.print(F(" Hundredths="));
    Serial.println(gps.time.centisecond());
  }

  else if (gps.speed.isUpdated())
  {
    Serial.print(F("SPEED      Fix Age="));
    Serial.print(gps.speed.age());
    Serial.print(F("ms Raw="));
    Serial.print(gps.speed.value());
    Serial.print(F(" Knots="));
    Serial.print(gps.speed.knots());
    Serial.print(F(" MPH="));
    Serial.print(gps.speed.mph());
    Serial.print(F(" m/s="));
    Serial.print(gps.speed.mps());
    Serial.print(F(" km/h="));
    Serial.println(gps.speed.kmph());
  }

  else if (gps.course.isUpdated())
  {
    Serial.print(F("COURSE     Fix Age="));
    Serial.print(gps.course.age());
    Serial.print(F("ms Raw="));
    Serial.print(gps.course.value());
    Serial.print(F(" Deg="));
    Serial.println(gps.course.deg());
  }

  else if (gps.altitude.isUpdated())
  {
    Serial.print(F("ALTITUDE   Fix Age="));
    Serial.print(gps.altitude.age());
    Serial.print(F("ms Raw="));
    Serial.print(gps.altitude.value());
    Serial.print(F(" Meters="));
    Serial.print(gps.altitude.meters());
    Serial.print(F(" Miles="));
    Serial.print(gps.altitude.miles());
    Serial.print(F(" KM="));
    Serial.print(gps.altitude.kilometers());
    Serial.print(F(" Feet="));
    Serial.println(gps.altitude.feet());
  }

  else if (gps.satellites.isUpdated())
  {
    Serial.print(F("SATELLITES Fix Age="));
    Serial.print(gps.satellites.age());
    Serial.print(F("ms Value="));
    Serial.println(gps.satellites.value());
  }

  else if (gps.hdop.isUpdated())
  {
    Serial.print(F("HDOP       Fix Age="));
    Serial.print(gps.hdop.age());
    Serial.print(F("ms raw="));
    Serial.print(gps.hdop.value());
    Serial.print(F(" hdop="));
    Serial.println(gps.hdop.hdop());
  }

  else if (millis() - last > 5000)
  {
    Serial.println();
    if (gps.location.isValid())
    {
      static const double LONDON_LAT = 51.508131, LONDON_LON = -0.128002;
      double distanceToLondon =
        TinyGPSPlus::distanceBetween(
          gps.location.lat(),
          gps.location.lng(),
          LONDON_LAT, 
          LONDON_LON);
      double courseToLondon =
        TinyGPSPlus::courseTo(
          gps.location.lat(),
          gps.location.lng(),
          LONDON_LAT, 
          LONDON_LON);

      Serial.print(F("LONDON     Distance="));
      Serial.print(distanceToLondon/1000, 6);
      Serial.print(F(" km Course-to="));
      Serial.print(courseToLondon, 6);
      Serial.print(F(" degrees ["));
      Serial.print(TinyGPSPlus::cardinal(courseToLondon));
      Serial.println(F("]"));
    }

    Serial.print(F("DIAGS      Chars="));
    Serial.print(gps.charsProcessed());
    Serial.print(F(" Sentences-with-Fix="));
    Serial.print(gps.sentencesWithFix());
    Serial.print(F(" Failed-checksum="));
    Serial.print(gps.failedChecksum());
    Serial.print(F(" Passed-checksum="));
    Serial.println(gps.passedChecksum());

    if (gps.charsProcessed() < 10)
      Serial.println(F("WARNING: No GPS data.  Check wiring."));

    last = millis();
    Serial.println();
  }
}
*/