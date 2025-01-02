#include "pico/rand.h"
#include <EEPROM.h>
// TODO: REVIEW / INTEGRATE overlap between sdhandler and Datalogger
#define EEPROM_TEST_FLAG_ADDR 0
#define EEPROM_SESSION_ID_ADDR 1
#define MAX_SESSION_ID 0xFFFF
#define FILE_ROLLOVER_LIMIT 1024 * 1024  // 1 MB per file

class Datalogger {
private:
  String sessionLabels;
  uint16_t session_id;
  bool test_in_progress;
  char index_file_name[13];
  char hardware_log_name[13];
  char data_file_name[13];
  uint32_t file_number;
  uint32_t file_size;
  uint32_t bytes_used_this_session;
  int16_t generateRandom();
  void updateFileName();
  void createNewDataFile();
  void appendIndexEntry(const char *entry);
  void appendHardwareLog(const char *message);
  void saveToEEPROM();
  void loadFromEEPROM();
  void clearEEPROM();

public:
  Datalogger();
  void setSessionLabels(const String &labels);
  bool init();
  bool startSession();
  void startNewSession();
  bool resumeSession();
  void stopSession();
  bool logData(const String &data);
  bool logHardwareEvent(const char *event);
};

Datalogger::Datalogger()
  : session_id(0), test_in_progress(false), file_number(0), file_size(0), bytes_used_this_session(0) {
  memset(index_file_name, 0, sizeof(index_file_name));
  memset(hardware_log_name, 0, sizeof(hardware_log_name));
  memset(data_file_name, 0, sizeof(data_file_name));
}

void Datalogger::setSessionLabels(const String &labels) {
  sessionLabels = labels;
}

int16_t Datalogger::generateRandom() {
  return static_cast<uint16_t>(get_rand_32());
}

void Datalogger::updateFileName() {
  sprintf(data_file_name, "%04X%04X.csv", session_id, file_number & 0xFFFF);
}

void Datalogger::createNewDataFile() {
  updateFileName();
  File dataFile = SD.open(data_file_name, FILE_WRITE);
  if (dataFile) {
    dataFile.println("Timestamp, Sensor1, Sensor2, Sensor3");  // Example headers
    dataFile.close();
    file_size = 0;
    appendIndexEntry(data_file_name);
  }
}

void Datalogger::appendIndexEntry(const char *entry) {
  File indexFile = SD.open(index_file_name, FILE_WRITE);
  if (indexFile) {
    indexFile.println(entry);
    indexFile.close();
  }
}

void Datalogger::appendHardwareLog(const char *message) {
  File logFile = SD.open(hardware_log_name, FILE_WRITE);
  if (logFile) {
    logFile.println(message);
    logFile.close();
  }
}

void Datalogger::saveToEEPROM() {
  EEPROM.write(EEPROM_TEST_FLAG_ADDR, test_in_progress ? 1 : 0);
  EEPROM.write(EEPROM_SESSION_ID_ADDR, session_id & 0xFF);
  EEPROM.write(EEPROM_SESSION_ID_ADDR + 1, (session_id >> 8) & 0xFF);
}

void Datalogger::loadFromEEPROM() {
  test_in_progress = EEPROM.read(EEPROM_TEST_FLAG_ADDR);
  session_id = EEPROM.read(EEPROM_SESSION_ID_ADDR) | (EEPROM.read(EEPROM_SESSION_ID_ADDR + 1) << 8);
}

void Datalogger::clearEEPROM() {
  EEPROM.write(EEPROM_TEST_FLAG_ADDR, 0);
  EEPROM.write(EEPROM_SESSION_ID_ADDR, 0);
  EEPROM.write(EEPROM_SESSION_ID_ADDR + 1, 0);
}

bool Datalogger::init() {
  loadFromEEPROM();
  return SD.begin();
}

bool Datalogger::startSession() {
  if (test_in_progress) return false;  // Already running a session

  session_id = random(1, MAX_SESSION_ID);
  test_in_progress = true;

  sprintf(index_file_name, "%04XINDX.TXT", session_id);
  sprintf(hardware_log_name, "%04XLOG.TXT", session_id);

  File indexFile = SD.open(index_file_name, FILE_WRITE);
  if (indexFile) {
    indexFile.print("File Rollover Limit: ");
    indexFile.println(FILE_ROLLOVER_LIMIT / 1024, DEC);
    indexFile.println("Associated Files:");
    indexFile.close();
  }

  appendHardwareLog("Session Started");
  saveToEEPROM();
  file_number = 0;
  createNewDataFile();

  return true;
}

void Datalogger::startNewSession() {
  session_id = generateRandom();
  sessionLabels = "Default Labels";
  Serial.printf("Started new session: %d\n", session_id);
}

bool Datalogger::resumeSession() {
  if (!test_in_progress) return false;

  sprintf(index_file_name, "%04XINDX.TXT", session_id);
  sprintf(hardware_log_name, "%04XLOG.TXT", session_id);

  appendHardwareLog("Session Resumed");
  return true;
}

void Datalogger::stopSession() {
  appendHardwareLog("Session Stopped");
  test_in_progress = false;
  clearEEPROM();
}

bool Datalogger::logData(const String &data) {
  if (!test_in_progress) return false;

  uint8_t data_size = data.length() + 1;
  if (file_size + data_size > FILE_ROLLOVER_LIMIT) {
    file_number++;
    createNewDataFile();
  }

  File dataFile = SD.open(data_file_name, FILE_WRITE);
  if (dataFile) {
    dataFile.println(data);
    dataFile.close();
    file_size += data_size;
    return true;
  }
  return false;
}

bool Datalogger::logHardwareEvent(const char *event) {
  appendHardwareLog(event);
  return true;
}
