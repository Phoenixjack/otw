#include <stdint.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BNO055.h>
#include <utility/imumaths.h>
#define BNO055_SAMPLERATE_DELAY_MS (100)

Adafruit_BNO055 bno = Adafruit_BNO055(276, 0x28);

struct bno055 {
  enum SENSOR_STATUS {
    NOT_INITIALIZED,
    INITIALIZED_NOT_CONFIGURED,
    CONFIGURED_NOT_CALIBRATED,
    CALIBRATED_AND_READY,
    CRITICAL_FAULT,
    NON_BLOCKING_FAULT,
    STATUS_END_OF_LIST
  };
  enum FAULT_CODES {
    NO_FAULT,
    FAULT_INITIALIZATION_FAILED,
    FAULT_CALIBRATION_FAILED,
    FAULT_SET_MODE_FAILED,
    FAULT_ILLEGAL_MODE_REQUEST,
    FAULT_FAILED_SELF_TEST,
    FAULT_END_OF_LIST
  };
  enum CALIBRATION_STATUS {
    AWAITING_CALIBRATION,
    NEEDS_CALIBRATION,
    CALIBRATION_IN_PROGRESS,
    CALIBRATION_SUCCESS,
    CALIBRATION_FAILED,
    CALIBRATION_END_OF_LIST
  };
  SENSOR_STATUS curr_status = NOT_INITIALIZED;
  FAULT_CODES curr_fault = NO_FAULT;
  CALIBRATION_STATUS cal_status = AWAITING_CALIBRATION;
  adafruit_bno055_opmode_t curr_mode = OPERATION_MODE_NDOF;
  uint32_t next_update = 0;   // For non-blocking calibration intervals
  char error_plain_text[21];  // For readable error/status messages
  uint32_t timestamp = 0;
  float x, y, z, heading;
  uint8_t tempC;
  bool init();                 // Perform basic initialization
  bool calibrate();            // Non-blocking calibration CHECK
  bool doCalibration();        // Non-blocking calibration
  bool check();                // Periodic health check
  void setMode(uint8_t mode);  // Set operating mode
  bool isTime();               // just checks if it's too soon
  void updateTime();           // updates the time trigger
  bool hasTriedToInit();
  bool hasInitFailed();
  bool hasCritFault();
  bool hasMinorFault();
  bool isInitialized();
  bool hasCalFailed();
  bool isCalibrated();
  bool hasAnyFault();
  bool isFaultCodeClear();
  bool isFullyReady();
} bno055;

bool bno055::init() {
  if (bno.begin()) {
    curr_fault = NO_FAULT;
    curr_status = INITIALIZED_NOT_CONFIGURED;
    cal_status = NEEDS_CALIBRATION;
    bno.setExtCrystalUse(true);
    delay(10);  // Ensure the crystal setting takes effect
    curr_mode = OPERATION_MODE_NDOF;
    setMode((uint8_t)curr_mode);
    strncpy(error_plain_text, "Initialized", sizeof(error_plain_text));
    curr_status = CONFIGURED_NOT_CALIBRATED;
    calibrate();
  } else {
    curr_fault = FAULT_INITIALIZATION_FAILED;
    curr_status = CRITICAL_FAULT;
    strncpy(error_plain_text, "Init Failed: Begin", sizeof(error_plain_text));
  }
  return isInitialized();
}

bool bno055::calibrate() {
  if (isTime() && isInitialized()) {
    switch (cal_status) {
      case AWAITING_CALIBRATION:
        strncpy(error_plain_text, "Not Ready for Cal", sizeof(error_plain_text));
        break;
      case NEEDS_CALIBRATION:
        doCalibration();
        break;
      case CALIBRATION_IN_PROGRESS:
        doCalibration();
        break;
      case CALIBRATION_SUCCESS:
        strncpy(error_plain_text, "Already Calibrated", sizeof(error_plain_text));
        break;
      case CALIBRATION_FAILED:
        doCalibration();
        break;
      default:
        strncpy(error_plain_text, "Unknown Cal State", sizeof(error_plain_text));
        break;
    }
    updateTime();
  }
  return isCalibrated();
}

bool bno055::doCalibration() {
  cal_status = CALIBRATION_IN_PROGRESS;
  uint8_t system, gyro, accel, mag;  // Fetch calibration data
  bno.getCalibration(&system, &gyro, &accel, &mag);
  snprintf(error_plain_text, sizeof(error_plain_text), "Cal: S=%d G=%d A=%d M=%d", system, gyro, accel, mag);
  if (bno.isFullyCalibrated()) {
    curr_status = CALIBRATED_AND_READY;
    cal_status = CALIBRATION_SUCCESS;
    curr_fault = NO_FAULT;
  }
  return isCalibrated();
}

bool bno055::check() {
  if (isTime() && isInitialized()) {
    uint8_t status, self_test, error;  // Perform a health check (e.g., status and error codes)
    bno.getSystemStatus(&status, &self_test, &error);
    if (error == 0) {
      sensors_event_t imu_event;
      bno.getEvent(&imu_event);
      timestamp = millis();
      x = imu_event.orientation.x;
      y = imu_event.orientation.y;
      z = imu_event.orientation.z;
      heading = imu_event.magnetic.heading;
      tempC = bno.getTemp();
      snprintf(error_plain_text, sizeof(error_plain_text), "Error: %d", error);
    } else {
      curr_status = NON_BLOCKING_FAULT;     // set the fault level
      curr_fault = FAULT_FAILED_SELF_TEST;  // update the reason
      cal_status = NEEDS_CALIBRATION;       // if we have a sensor failure, then invalidate the calibration
      snprintf(error_plain_text, sizeof(error_plain_text), "Error: %d", error);
    }
    updateTime();
  } else {
    if (!isInitialized()) { strncpy(error_plain_text, "Not Ready", sizeof(error_plain_text)); }
  }
  return isFullyReady();
}

void bno055::setMode(uint8_t requested_mode) {
  if (requested_mode > 12) {
    snprintf(error_plain_text, sizeof(error_plain_text), "Invalid Mode %d", requested_mode);  // if we got an invalid mode, set an error notice, but don't interrupt operations
  } else {
    bno.setMode(OPERATION_MODE_CONFIG);  // Enter config mode
    delay(25);
    bno.setMode((adafruit_bno055_opmode_t)requested_mode);  // Set desired mode
    delay(25);
    curr_mode = bno.getMode();
    snprintf(error_plain_text, sizeof(error_plain_text), "Mode Set: %d", curr_mode);
  }
}

bool bno055::isTime() {
  return (millis() > next_update);
}

void bno055::updateTime() {
  next_update = millis() + BNO055_SAMPLERATE_DELAY_MS;
}

//  enum SENSOR_STATUS {NOT_INITIALIZED,INITIALIZED_NOT_CONFIGURED,CONFIGURED_NOT_CALIBRATED,CALIBRATED_AND_READY,CRITICAL_FAULT,NON_BLOCKING_FAULT,
bool bno055::hasTriedToInit() {
  return (curr_status != NOT_INITIALIZED);  // Once init() has been called once, curr_status will never go back to NOT_INITIALIZED
}

bool bno055::hasCritFault() {
  return (curr_status == CRITICAL_FAULT);
}

bool bno055::hasMinorFault() {
  return (curr_status == NON_BLOCKING_FAULT);
}

//  enum FAULT_CODES {NO_FAULT,FAULT_INITIALIZATION_FAILED,FAULT_CALIBRATION_FAILED,FAULT_SET_MODE_FAILED,FAULT_ILLEGAL_MODE_REQUEST,FAULT_FAILED_SELF_TEST,
bool bno055::isFaultCodeClear() {  // be cautious as overlap between this & hasCritFault() || hasMinorFault()
  return (curr_fault == NO_FAULT);
}

bool bno055::hasInitFailed() {
  return (curr_fault == FAULT_INITIALIZATION_FAILED);
}

//  enum CALIBRATION_STATUS { AWAITING_CALIBRATION,NEEDS_CALIBRATION,CALIBRATION_IN_PROGRESS,CALIBRATION_SUCCESS,CALIBRATION_FAILED,
bool bno055::isCalibrated() {
  return cal_status == CALIBRATION_SUCCESS;
}

bool bno055::hasCalFailed() {
  return (cal_status == CALIBRATION_FAILED);
}

// COMBINES REFERENCES TO ALL 3 FLAGS
bool bno055::isInitialized() {
  return hasTriedToInit() && !hasInitFailed() && !hasCritFault();
}

bool bno055::hasAnyFault() {
  return hasCritFault() || hasMinorFault() || !isFaultCodeClear();
}

bool bno055::isFullyReady() {
  return isCalibrated() && isFaultCodeClear();
}