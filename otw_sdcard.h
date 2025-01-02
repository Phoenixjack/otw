#include "pico/platform/compiler.h"
#include "pico/rand.h"

#include "hardware/address_mapped.h"
#define PIN_SD_MOSI PIN_SPI0_MOSI
#define PIN_SD_MISO PIN_SPI0_MISO
#define PIN_SD_SCK PIN_SPI0_SCK
#define PIN_SD_SS PIN_SPI0_SS

#include <SPI.h>
#include <RP2040_SD.h>  // https://github.com/khoih-prog/RP2040_SD

Sd2Card card;
SdVolume volume;
File root;
File myFile;

struct sdhandler {
public:
  enum SD_STATUS_CODE {
    SD_STATUS_NOT_STARTED,
    SD_STATUS_INITIALIZING_HW,
    SD_STATUS_INITIALIZED_HW,
    SD_STATUS_MOUNTED_CARD,
    SD_STATUS_MOUNTED_FS,
    SD_STATUS_READY,
    SD_STATUS_CRITICAL_ERROR,
    SD_STATUS_MINOR_ERROR,
    SD_STATUS_END_OF_LIST
  };
  enum SD_LAST_ACTION {
    SD_LAST_NO_ACTION_YET,
    SD_LAST_INDEX_FILE_WRITTEN,
    SD_LAST_INDEX_UPDATED,
    SD_LAST_OPENED_FOR_WRITE,
    SD_LAST_END_OF_LIST
  };
  enum SD_FAULT_STATE {
    SD_FAULT_NO_FAULT,
    SD_FAULT_STATUS_NOT_FORMATTED,
    SD_FAULT_STATUS_FULL,
    SD_FAULT_CANNOT_INITIALIZE_HW,
    SD_FAULT_COULD_NOT_MOUNT_CARD,
    SD_FAULT_COULD_NOT_MOUNT_FILESYSTEM,
    SD_FAULT_COULD_NOT_OPEN_DATA_FILE,
    SD_FAULT_COULD_NOT_CREATE_NEW_DATA_FILE,
    SD_FAULT_INDEX_NOT_UPDATED,
    SD_FAULT_UNABLE_OPEN_FOR_WRITE,
    SD_FAULT_UNABLE_CLOSE_FILE,
    SD_FAULT_FILE_NOT_EXIST,
    SD_FAULT_END_OF_LIST
  };
  SD_STATUS_CODE curr_status = SD_STATUS_NOT_STARTED;
  SD_LAST_ACTION last_action = SD_LAST_NO_ACTION_YET;
  SD_FAULT_STATE curr_fault = SD_FAULT_NO_FAULT;

  float disk_Space_Used_perc = 0;
  bool hasCritFault();
  bool hasMinorFault();
  bool isReady();
  bool init();
  bool initHardware();  // SD.begin() Initializes the SD card hardware
  bool mountCard();     // card.mount() Mounts the SD card inserted into the hardware
  bool initFS();        // volume.init() Initializes and validates the filesystem.

  void startSession(int16_t sessionId = 0);
  void startNewSession();
  void startNewSession(String userLabels);
  uint16_t getSessionID();
  bool isCurrFileFull(uint8_t newDataSize);
  bool markTime(uint32_t timestamp);
  bool writeData(String* dataToSave);
  bool writeData(String dataToSave);
  uint32_t getCurrFileNum();
  float getVolumeSizeGB();
  uint32_t getFreeSizeKB();
  void printDirectory(File dir, int numTabs);
  void printAll();
  bool readFile(String targetFileName);
  void readCurrentFile();
  String getCurrFileName();

private:
  // Card and volume information
  uint32_t volumeSizeKB = 0;
  uint32_t volumeUsedKB = 0;
  uint32_t volumeFreeKB = 0;
  uint16_t directoryCount = 0;
  uint16_t fileCount = 0;
  uint32_t fileSizeMaxBytes = 102400;
  uint8_t columnLabelsLength = 0;
  uint32_t fileSizeDataBytes = 0;

  // Session information
  uint16_t sessionId = 0;
  uint32_t currentFileNum = 0;
  char columnLabels[256];

  // File names
  char fileNameIndex[13];
  char fileNameTime[13];
  char fileNameData[13];
  uint32_t bytesUsedThisSession = 0;

  // Time management
  bool timeNotSaved = true;
  uint32_t lastTimeSync = 0;
  uint32_t bootTimeSec = 0;
  int16_t bootDriftSec = 0;

  // Private methods
  bool startUp();
  bool initializeCard();
  bool mountVolume();
  void updateFileName();
  void initializeIndexFile();
  void updateIndexFile();
  void startNewFile();
  void calculateCardSpace();
  void calculateUsedSpace(File dir);
  int16_t generateRandom();
  bool doesFileExist(const char* fileNameInput);
  float convertKBtoGB(uint32_t kb);

  // Helper methods
  void handleFileCreationError();
};

//   enum SD_STATUS_CODE {SD_STATUS_NOT_STARTED,SD_STATUS_INITIALIZING_HW,SD_STATUS_INITIALIZED_HW,SD_STATUS_MOUNTED_CARD,SD_STATUS_MOUNTED_FS,
//   SD_STATUS_READY,SD_STATUS_CRITICAL_ERROR,SD_STATUS_MINOR_ERROR,,SD_STATUS_END_OF_LIST};

//   enum SD_FAULT_STATE {SD_FAULT_NO_FAULT,SD_FAULT_STATUS_NOT_FORMATTED,SD_FAULT_COULD_NOT_MOUNT_CARD,SD_FAULT_COULD_NOT_MOUNT_FILESYSTEM,SD_FAULT_STATUS_FULL,
//   SD_FAULT_CANNOT_INITIALIZE_HW,SD_FAULT_COULD_NOT_OPEN_DATA_FILE,SD_FAULT_COULD_NOT_CREATE_NEW_DATA_FILE,SD_FAULT_INDEX_NOT_UPDATED,SD_FAULT_UNABLE_OPEN_FOR_WRITE,
//   SD_FAULT_UNABLE_CLOSE_FILE,SD_FAULT_FILE_NOT_EXIST,SD_FAULT_END_OF_LIST};

//   enum SD_LAST_ACTION {SD_LAST_NO_ACTION_YET,SD_LAST_INDEX_FILE_WRITTEN,SD_LAST_INDEX_UPDATED,SD_LAST_OPENED_FOR_WRITE,SD_LAST_END_OF_LIST};

bool sdhandler::hasCritFault() {
  return (curr_status == SD_STATUS_CRITICAL_ERROR);
}
bool sdhandler::hasMinorFault() {
  return (curr_status == SD_STATUS_MINOR_ERROR);
}

bool sdhandler::isReady() {
  return (curr_status == SD_STATUS_READY);
}

bool sdhandler::init() {
  curr_status = SD_STATUS_INITIALIZING_HW;
  if (initHardware()) {
    if (mountCard()) {
      if (initFS()) {
        curr_status = SD_STATUS_READY;
        curr_fault = SD_FAULT_NO_FAULT;
        calculateCardSpace();  // can cause a fault if usage > 95%
      }
    }
  }
  return isReady();
}

bool sdhandler::initHardware() {
  if (SD.begin(PIN_SD_SS)) {
    curr_status = SD_STATUS_INITIALIZED_HW;
  } else {
    curr_status = SD_STATUS_CRITICAL_ERROR;
    curr_fault = SD_FAULT_CANNOT_INITIALIZE_HW;
  }
  return !hasCritFault();
}

bool sdhandler::mountCard() {
  if (card.init(SPI_HALF_SPEED, PIN_SD_SS)) {
    curr_status = SD_STATUS_MOUNTED_CARD;
  } else {
    curr_status = SD_STATUS_CRITICAL_ERROR;
    curr_fault = SD_FAULT_COULD_NOT_MOUNT_CARD;
  }
  return !hasCritFault();
}

bool sdhandler::initFS() {
  if (volume.init(card)) {
    volumeSizeKB = (volume.blocksPerCluster() * volume.clusterCount() / 2);
    curr_status = SD_STATUS_MOUNTED_FS;
  } else {
    curr_status = SD_STATUS_CRITICAL_ERROR;
    curr_fault = SD_FAULT_COULD_NOT_MOUNT_FILESYSTEM;
  }
  return !hasCritFault();
}

void sdhandler::calculateCardSpace() {
  if (volumeSizeKB > 0) {
    volumeUsedKB = 0;
    directoryCount = 0;
    fileCount = 0;
    root = SD.open("/");
    calculateUsedSpace(root);
    volumeFreeKB = volumeSizeKB - volumeUsedKB;
    disk_Space_Used_perc = 100 * volumeUsedKB / volumeSizeKB;
    if (disk_Space_Used_perc > 95) {
      curr_status = SD_STATUS_CRITICAL_ERROR;
      curr_fault = SD_FAULT_STATUS_FULL;
    }
  }
}

void sdhandler::calculateUsedSpace(File dir) {
  while (true) {
    File entry = dir.openNextFile();
    if (!entry) { break; }
    if (entry.isDirectory()) {
      directoryCount++;
      calculateUsedSpace(entry);
    } else {
      fileCount++;
      volumeUsedKB += entry.size();
    }
    entry.close();
  }
}

float sdhandler::convertKBtoGB(uint32_t kb) {
  return static_cast<float>(kb) / 1048576;
}

int16_t sdhandler::generateRandom() {
  return static_cast<uint16_t>(get_rand_32());
}

bool sdhandler::doesFileExist(const char* fileNameInput) {
  return SD.exists(fileNameInput);
}
