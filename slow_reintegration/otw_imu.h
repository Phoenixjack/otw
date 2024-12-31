#include <stdint.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BNO055.h>
#include <utility/imumaths.h>
#define BNO055_SAMPLERATE_DELAY_MS (100)

Adafruit_BNO055 bno = Adafruit_BNO055(276, 0x28);

struct imu_data {
  static sensors_event_t imu_event;  // Make imu_event static
  struct calibration {
    uint32_t timestamp = 0;
    uint8_t system, gyro, accel, mag;
    void update(bool solo);
  } cal;
  struct orient {
    uint32_t timestamp = 0;
    float x, y, z, heading;
    uint8_t tempC;
    void update(bool solo);
  } orient;
  struct system {
    uint32_t timestamp = 0;
    uint8_t status, self_test_results, error;
    void update(bool solo);
  } system;
  void update_all();
} imu_data;  // Declare a single global instance named imu_data

sensors_event_t imu_data::imu_event;  // Definition of the static imu_event outside the struct

void imu_data::calibration::update(bool solo = true) {
  if (solo) { bno.getEvent(&imu_event); }
  timestamp = millis();
  bno.getCalibration(&system, &gyro, &accel, &mag);
};

void imu_data::orient::update(bool solo = true) {
  if (solo) { bno.getEvent(&imu_event); }
  timestamp = millis();
  x = imu_event.orientation.x;
  y = imu_event.orientation.y;
  z = imu_event.orientation.z;
  heading = imu_event.magnetic.heading;
  tempC = bno.getTemp();
};

void imu_data::system::update() {
  if (solo) { bno.getEvent(&imu_event); }
  timestamp = millis();
  bno.getSystemStatus(&status, &self_test_results, &error);
};

imu_data::update_all() {
  bno.getEvent(&imu_event);  // Update imu_event once
  cal.update(false);         // No need to fetch imu_event again
  orient.update(false);      // No need to fetch imu_event again
  system.update(false);      // No need to fetch imu_event again
};

struct bno055 {
  enum STATUS {  // TODO: There's a lot of overlap between the curr_status code and the ready/fault/calibrated booleans.
    NOT_INITIALIZED,
    INITIALIZED_NOT_CALIBRATED,
    INITIALIZED_AND_CALIBRATED,
    CALIBRATION_IN_PROGRESS,
    FAULT_INITIALIZATION_FAILED,
    FAULT_NOT_DETECTED,
    FAULT_NOT_CALIBRATED,
    FAULT_ILLEGAL_MODE_REQUEST,
    FAULT_TBD,
    STATUS_END_OF_LIST
  };
  STATUS curr_status = NOT_INITIALIZED;
  adafruit_bno055_opmode_t curr_mode = OPERATION_MODE_NDOF;  // OPTIONS: OPERATION_MODE_CONFIG, OPERATION_MODE_ACCONLY, OPERATION_MODE_MAGONLY, OPERATION_MODE_GYRONLY, OPERATION_MODE_ACCMAG, OPERATION_MODE_ACCGYRO,
                                                             // OPERATION_MODE_MAGGYRO, OPERATION_MODE_AMG, OPERATION_MODE_IMUPLUS, OPERATION_MODE_COMPASS, OPERATION_MODE_M4G, OPERATION_MODE_NDOF_FMC_OFF, OPERATION_MODE_NDOF
  bool ready, fault, calibrated;                             // TODO: should probably get rid of this
  uint32_t next_update;
  void update_time();
  bool init();   // will return false only if it's not ready
  bool check();  // will return false only if there's an error
  bool setMode(uint8_t requested_mode);
  bool tryingToCalibrate();
  bool isUncalibrated();
  bool isFullyReady();
  bool isFaulty();
} bno055;

void bno055::update_time() {
  next_update = millis() + BNO055_SAMPLERATE_DELAY_MS;
};

bool bno055::init() {
  ready = bno.begin();
  if (ready) {
    curr_status = INITIALIZED_NOT_CALIBRATED;
    delay(200);
    bno.setExtCrystalUse(true);
    curr_mode = OPERATION_MODE_COMPASS;
    ready = setMode((uint8_t)curr_mode);
    calibrated = bno.isFullyCalibrated();
    if (calibrated) { curr_mode = INITIALIZED_AND_CALIBRATED; }
    ready = isFullyReady();
    update_time();
  } else {
    next_update = millis() + 10000;  // set a retry time
    curr_status = FAULT_INITIALIZATION_FAILED;
    fault = true;
  }
  return ready;
};

bool bno055::check() {  // need to add a line in here to check calibration and start that if needed
  if (ready && millis() > next_update) {
    imu_data.update_all();
    fault = false;
    update_time();
  } else {
    if (!ready) { fault = true; }
  }
  return !fault;
};

bool bno055::setMode(uint8_t requested_mode) {  // INCOMPLETE: Certain invalid values can initiate recalls of cal data
  char buffer[200];
  if (requested_mode < 13) {
    sprintf(buffer, "SETTING MODE: [%i] \t", requested_mode);
    Serial.print(buffer);
    bno.setMode(OPERATION_MODE_CONFIG);                     // Enter config mode
    delay(25);                                              // Short delay for the mode transition
    bno.setMode((adafruit_bno055_opmode_t)requested_mode);  // Set the desired mode
    delay(25);                                              // Allow the mode change to take effect
    curr_mode = bno.getMode();
    sprintf(buffer, "[CURRENT MODE:%i]", curr_mode);
    Serial.println(buffer);
    return true;
  } else if (requested_mode == 13) {
    Serial.println("Attempting to reload cal data");
    adafruit_bno055_offsets_t calibrationData;
    bno.getSensorOffsets(calibrationData);  // ******************************* INCOMPLETE *******************************
    bno.setSensorOffsets(calibrationData);
    return true;
  } else {
    curr_status = FAULT_ILLEGAL_MODE_REQUEST;
    sprintf(buffer, "ILLEGAL MODE REQUEST [%i]", requested_mode);
    Serial.println(buffer);
    return false;
  }
};

bool bno055::tryingToCalibrate() {
  return (curr_status == CALIBRATION_IN_PROGRESS);
};

bool bno055::isUncalibrated() {
  return (curr_status == CALIBRATION_IN_PROGRESS || curr_status == INITIALIZED_NOT_CALIBRATED);
};

bool bno055::isFullyReady() {
  return (curr_status == INITIALIZED_AND_CALIBRATED);
};

bool bno055::isFaulty() {
  return !(curr_status == INITIALIZED_AND_CALIBRATED || curr_status == INITIALIZED_NOT_CALIBRATED);
};




/*
  sensors_event_t orientationData , angVelocityData , linearAccelData, magnetometerData, accelerometerData, gravityData;
  bno.getEvent(&orientationData, Adafruit_BNO055::VECTOR_EULER);
  bno.getEvent(&angVelocityData, Adafruit_BNO055::VECTOR_GYROSCOPE);
  bno.getEvent(&linearAccelData, Adafruit_BNO055::VECTOR_LINEARACCEL);
  bno.getEvent(&magnetometerData, Adafruit_BNO055::VECTOR_MAGNETOMETER);
  bno.getEvent(&accelerometerData, Adafruit_BNO055::VECTOR_ACCELEROMETER);
  bno.getEvent(&gravityData, Adafruit_BNO055::VECTOR_GRAVITY);
  printEvent(&orientationData);
  printEvent(&angVelocityData);
  printEvent(&linearAccelData);
  printEvent(&magnetometerData);
  printEvent(&accelerometerData);
  printEvent(&gravityData);

  void printEvent(sensors_event_t* event) {
  double x = -1000000, y = -1000000 , z = -1000000; //dumb values, easy to spot problem
  if (event->type == SENSOR_TYPE_ACCELEROMETER) {
    Serial.print("Accl:");
    x = event->acceleration.x;
    y = event->acceleration.y;
    z = event->acceleration.z;
  }
  else if (event->type == SENSOR_TYPE_ORIENTATION) {
    Serial.print("Orient:");
    x = event->orientation.x;
    y = event->orientation.y;
    z = event->orientation.z;
  }
  else if (event->type == SENSOR_TYPE_MAGNETIC_FIELD) {
    Serial.print("Mag:");
    x = event->magnetic.x;
    y = event->magnetic.y;
    z = event->magnetic.z;
  }
  else if (event->type == SENSOR_TYPE_GYROSCOPE) {
    Serial.print("Gyro:");
    x = event->gyro.x;
    y = event->gyro.y;
    z = event->gyro.z;
  }
  else if (event->type == SENSOR_TYPE_ROTATION_VECTOR) {
    Serial.print("Rot:");
    x = event->gyro.x;
    y = event->gyro.y;
    z = event->gyro.z;
  }
  else if (event->type == SENSOR_TYPE_LINEAR_ACCELERATION) {
    Serial.print("Linear:");
    x = event->acceleration.x;
    y = event->acceleration.y;
    z = event->acceleration.z;
  }
  else if (event->type == SENSOR_TYPE_GRAVITY) {
    Serial.print("Gravity:");
    x = event->acceleration.x;
    y = event->acceleration.y;
    z = event->acceleration.z;
  }

  int8_t boardTemp = bno.getTemp();
  bno.getCalibration(&system, &gyro, &accel, &mag);
*/
