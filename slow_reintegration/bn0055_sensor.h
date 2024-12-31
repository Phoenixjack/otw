#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BNO055.h>  // https://github.com/adafruit/Adafruit_BNO055
#include <utility/imumaths.h>
// CALIBRATION: https://cdn-learn.adafruit.com/assets/assets/000/125/776/original/bst-bno055-ds000.pdf?1698865246 PAGE 51
Adafruit_BNO055 bno = Adafruit_BNO055(276, 0x28);  // https://www.adafruit.com/product/2472

struct bn0055 {
  bool ready = false;
  bool calibrated = false;
  uint8_t min_read_interval_ms = 100;
  uint32_t next_read = 0;
  uint32_t last_read = 0;
  float x, y, z;
  uint8_t sys, gyro, accel, mag = 0;
  uint8_t consecutive_zero_detections = 0;
  bool init();
  bool check(bool get_cal_data);
  void displaySensorDetails();
} bn0055;

bool bn0055::init() {
  consecutive_zero_detections = 0;
  if (bno.begin()) {
    ready = true;
    bno.setExtCrystalUse(true);
    bno.getCalibration(&sys, &gyro, &accel, &mag);
  } else {
    ready = false;
  }
  last_read = millis();
  next_read = last_read + (uint32_t)min_read_interval_ms; // we'll set this here so that we can re-run init periodically in the main loop
  return ready;
};

bool bn0055::check(bool get_cal_data = false) {
  if (millis() > next_read && ready) {
    sensors_event_t event;
    bno.getEvent(&event);
    x = (float)event.orientation.x;
    y = (float)event.orientation.y;
    z = (float)event.orientation.z;
    if (x == 0) {
      consecutive_zero_detections++;
      if (consecutive_zero_detections > (2000 / min_read_interval_ms)) { ready = false; }  // we're probably in G-lock. invalidate the ready status so it'll be rebooted/reinitialized
    } else {
      if (consecutive_zero_detections > 0) { consecutive_zero_detections = 0; }  // we might've gotten several consecutive zeroes, but it appears to have cleared
    }
    if (get_cal_data) {
      bno.getCalibration(&sys, &gyro, &accel, &mag);
    }
    last_read = millis();
    next_read = last_read + (uint32_t)min_read_interval_ms;
    return true;
  } else {
    return false;
  }
};

void bn0055::displaySensorDetails() {
  sensor_t sensor;
  bno.getSensor(&sensor);
  Serial.println("\n\n\n------------------------------------");
  Serial.print("Sensor:       ");
  Serial.println(sensor.name);
  Serial.print("Driver Ver:   ");
  Serial.println(sensor.version);
  Serial.print("Unique ID:    ");
  Serial.println(sensor.sensor_id);
  Serial.print("Max Value:    ");
  Serial.print(sensor.max_value);
  Serial.println(" xxx");
  Serial.print("Min Value:    ");
  Serial.print(sensor.min_value);
  Serial.println(" xxx");
  Serial.print("Resolution:   ");
  Serial.print(sensor.resolution);
  Serial.println(" xxx");
  Serial.println("------------------------------------");
  Serial.println("");
}

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
