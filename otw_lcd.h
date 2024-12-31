/*
  case MODE_BOOT: // flag each line with REQUIRED or OPTIONAL
    0. bn55
    1. gps
    2. sd card
    3. data packet in
  case MODE_RUNTIME:
    0. Overall system line: uptime, data points saved/sent, heading/temp, gps fixquality/age
    1. gps lat/long, orient xyz
    2. sd card usage %, filenum / num data points, last time sync
    3. raw packet
  case MODE_ERROR:
    0. Overall system line: uptime, data points saved/sent, heading/temp, gps fixquality/age
    1. alert level, subsystem name
    2. diagnostics...
    3. diagnostics...
*/

#include <array>
#include <string>
#include <LiquidCrystal_I2C.h>
#define LCD_COLUMNS 20
#define LCD_ROWS 4
LiquidCrystal_I2C lcd(0x27, LCD_COLUMNS, LCD_ROWS);
struct disp {
  enum DISP_MODE {
    MODE_BOOT,
    MODE_RUNTIME,
    MODE_ERROR,
    MODE_END_END_OF_LIST
  };
  DISP_MODE curr_disp_mode = MODE_END_END_OF_LIST;
  enum LIST_INFO {  // TODO: FILL OUT THESE DISPLAY ROUTINES!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
    LIST_INFO_status_bno055,
    LIST_INFO_status_gps,
    LIST_INFO_status_uart_in,
    LIST_INFO_status_sdcard,
    LIST_INFO_status_dataout,  // placeholder for lora/wifi
    LIST_INFO_data_lat_long,
    LIST_INFO_data_gpsfix,
    LIST_INFO_data_millistime,
    LIST_INFO_data_epochtime,
    LIST_INFO_data_humantime,
    LIST_INFO_data_bno055_orientation,
    LIST_INFO_data_bno055_calibration,
    LIST_INFO_data_raw_uart,
    LIST_INFO_data_packet_stats,
    LIST_INFO_data_sdcard_stats,
    LIST_INFO_data_data_sent,
    LIST_INFO_END_OF_LIST
  };

  bool enabled = true;
  uint32_t last_update, next_update;  // TODO: last_update is redundant unless we implement a sleep mode
  uint32_t refresh_interval_ms = 250;
  char text[LCD_ROWS][LCD_COLUMNS + 1];

  struct line {
    uint32_t next_change = 0;
    uint8_t list_index = 0;                                                                                                          // Start with the first mode
    std::array<LIST_INFO, 4> list = { LIST_INFO_END_OF_LIST, LIST_INFO_END_OF_LIST, LIST_INFO_END_OF_LIST, LIST_INFO_END_OF_LIST };  // FYI: max of 4 items for each line to cycle through
    std::array<uint8_t, 4> hold_time_in_secs = { 0, 0, 0, 0 };                                                                       // FYI: arbitrary decision on my part
    void check_mode();
    LIST_INFO getCurrMode();
  } line[LCD_ROWS];

  void init();
  void setDispMode(DISP_MODE newmode);
  void updateBuffer(uint8_t thisrow);
  void update();
  void redraw();

  void drawStatus(thisrow);
  void drawOrientation(uint8_t rownum);
  void drawCalibration(thisrow);
  void drawSystem(thisrow);
  void drawTime(thisrow);
  void drawError(thisrow);
} disp;


void disp::line::check_mode() {  // if it's time, then we'll cycle to the next valid mode and set the next trigger time
  if (millis() > next_change) {
    do {
      list_index++;
      if (list_index == list.size()) { list_index = 0; }  // End of index, cycle back around
    } while (list[list_index] == LIST_INFO_END_OF_LIST || hold_time_in_secs[list_index] == 0);
    next_change = millis() + ((uint32_t)hold_time_in_secs[list_index] * 1000);
  }
};

disp::line::LIST_INFO disp::line::getCurrMode() {  // just for cleanliness of coding
  check_mode();
  return list[list_index];
}

void disp::init() {  // initializes the LCD and sets up our mode
  lcd.init();
  lcd.backlight();
  lcd.clear();
  setDispMode(MODE_BOOT);
  last_update = millis();  // TODO: remove?
  next_update = last_update + refresh_interval_ms;
};

void disp::update() {           // TODO: NEED to roll up our readiness state for all required subsystems, NOT just the heading!
  if (bno055.isFullyReady()) {  // first check our mode
    setDispMode(MODE_RUNTIME);  // IFF we're coming out of bootmode OR we've recovered from an error, then set up scrolling
  } else {
    setDispMode(MODE_ERROR);  // IF we have a problem, immediately override bootmode display
  }
  if (millis() > next_update) {
    redraw();
    last_update = millis();
    next_update = last_update + refresh_interval_ms;
  }
};

void disp::setDispMode(DISP_MODE newmode) {  // will evaluate curr_disp_mode before taking action
  if (curr_disp_mode != newmode) {           // IFF it's a mode change
    lcd.clear();                             // then we clear screen
    curr_disp_mode = newmode;                // save it
    switch (curr_disp_mode) {                // and apply new settings
      case MODE_BOOT:
        line[0].list = { LIST_INFO_status, LIST_INFO_END_OF_LIST, LIST_INFO_END_OF_LIST };
        line[0].hold_time_in_secs = { 30, 0, 0, 0 };  //
        line[1].list = { LIST_INFO_calibration, LIST_INFO_END_OF_LIST, LIST_INFO_END_OF_LIST };
        line[1].hold_time_in_secs = { 30, 0, 0, 0 };  //
        line[2].list = { LIST_INFO_orientation, LIST_INFO_END_OF_LIST, LIST_INFO_END_OF_LIST };
        line[2].hold_time_in_secs = { 30, 0, 0, 0 };  //
        line[3].list = { LIST_INFO_time, LIST_INFO_END_OF_LIST, LIST_INFO_END_OF_LIST };
        line[3].hold_time_in_secs = { 30, 0, 0, 0 };  //
        break;
      case MODE_RUNTIME:
        line[0].list = { LIST_INFO_system, LIST_INFO_epoch, LIST_INFO_humantime };                  // OVERALL STATUS...    LIST_INFO_data_millistime, LIST_INFO_data_epochtime, LIST_INFO_data_humantime,
        line[0].hold_time_in_secs = { 5, 5, 5 };                                                    //
        line[1].list = { LIST_INFO_raw_uart, LIST_INFO_packet_stats, LIST_INFO_data_sent };         // LIST_INFO_status_sdcard  LIST_INFO_status_bno055 LIST_INFO_status_gps LIST_INFO_status_uart_in
        line[1].hold_time_in_secs = { 10, 10, 5 };                                                  //
        line[2].list = { LIST_INFO_orientation, LIST_INFO_lat_long, LIST_INFO_fixdata };            // GPS / positioning
        line[2].hold_time_in_secs = { 4, 4, 2 };                                                    //
        line[3].list = { LIST_INFO_sdcard_status, LIST_INFO_sdcard_stats, LIST_INFO_END_OF_LIST };  // SD Card and file saving LIST_INFO_data_sdcard_stats
        line[3].hold_time_in_secs = { 10, 15, 0 };                                                  //
        break;
      case MODE_ERROR:  // overrides runtime scrolling to lock in an error TODO: add conditionals to lock into that subsystem's error
        line[0].list = { LIST_INFO_status, LIST_INFO_END_OF_LIST, LIST_INFO_END_OF_LIST, LIST_INFO_END_OF_LIST };
        line[0].hold_time_in_secs = { 30, 0, 0, 0 };  // overall status. is this a critical or a minor subsystem? If non-critical, then status of ongoing processing
        line[1].list = { LIST_INFO_calibration, LIST_INFO_END_OF_LIST, LIST_INFO_END_OF_LIST, LIST_INFO_END_OF_LIST };
        line[1].hold_time_in_secs = { 30, 0, 0, 0 };  // error; which system?
        line[2].list = { LIST_INFO_orientation, LIST_INFO_END_OF_LIST, LIST_INFO_END_OF_LIST, LIST_INFO_END_OF_LIST };
        line[2].hold_time_in_secs = { 30, 0, 0, 0 };  // mode
        line[3].list = { LIST_INFO_error, LIST_INFO_END_OF_LIST, LIST_INFO_END_OF_LIST, LIST_INFO_END_OF_LIST };
        line[3].hold_time_in_secs = { 30, 0, 0, 0 };  // diagnostics
        break;
      default:
        Serial.println(F("Failure to set DISP_MODE"));
        break;
    }
  }
};

void disp::redraw() {  // cycles through each line once all checks have been passed AND line assignments made; calls updateBuffer()
  for (uint8_t i = 0; i < LCD_ROWS; i++) {
    updateBuffer(i);
    lcd.print(text[i]);
  }
};

void disp::updateBuffer(uint8_t thisrow) {  // switch case for routing data to the correct line
  /*
    LIST_INFO_status_bno055,
    LIST_INFO_status_gps,
    LIST_INFO_status_uart_in,
    LIST_INFO_status_sdcard,
    LIST_INFO_status_dataout,  // placeholder for lora/wifi
    LIST_INFO_data_lat_long,
    LIST_INFO_data_gpsfix,
    LIST_INFO_data_millistime,
    LIST_INFO_data_epochtime,
    LIST_INFO_data_humantime,
    LIST_INFO_data_bno055_orientation,
    LIST_INFO_data_bno055_calibration,
    LIST_INFO_data_raw_uart,
    LIST_INFO_data_packet_stats,
    LIST_INFO_data_sdcard_stats,
    LIST_INFO_data_data_sent,
    LIST_INFO_END_OF_LIST

  switch (line[thisrow].getCurrMode()) {
    case LIST_INFO_status:
      bufferStatus(thisrow);
      break;
    case LIST_INFO_orientation:
      bufferOrientation(thisrow);
      break;
    case LIST_INFO_calibration:
      bufferCalibration(thisrow);
      break;
    case LIST_INFO_system:
      bufferSystem(thisrow);
      break;
    case LIST_INFO_time:
      bufferTime(thisrow);
      break;
    case LIST_INFO_error:
      bufferError(thisrow);
      break;
    default:
      // Handle invalid mode
      break;
  }
      */
};










void disp::bufferStatus(thisrow) {  // TODO: work in progress err... trainwreck
  switch (bno055.curr_status) {     // OPTIONS: CALIBRATION_IN_PROGRESS, FAULT_NOT_DETECTED, FAULT_NOT_CALIBRATED, FAULT_ILLEGAL_MODE_REQUEST, FAULT_TBD, STATUS_END_OF_LIST
    case bno055::INITIALIZED_AND_CALIBRATED:
      snprintf(text[rownum], sizeof(text[rownum]), "READY MODE[%01X]", bno055.curr_mode);
      break;
    case bno055::INITIALIZED_NOT_CALIBRATED:
      snprintf(text[rownum], sizeof(text[rownum]), "NoCal MODE[%01X]", bno055.curr_mode);
      break;
    case bno055::NOT_INITIALIZED:
      snprintf(text[rownum], sizeof(text[rownum]), "STAT: %01X", bno055.curr_status);
      break;
    case bno055::FAULT_INITIALIZATION_FAILED:
      snprintf(text[rownum], sizeof(text[rownum]), "InitFAIL %04.2f", (float)(bno055.next_update - millis()) / 1000);
      break;
    default:
      snprintf(text[rownum], sizeof(text[rownum]), "ERROR: %01X", bno055.curr_status);
      break;
  }
};

void disp::bufferOrientation(uint8_t rownum) {  // gtg
  snprintf(text[rownum], sizeof(text[rownum]), "%06.2f %06.2f %06.2f", imu_data.orient.x, imu_data.orient.y, imu_data.orient.z);
};

void disp::bufferCalibration(uint8_t rownum) {  // gtg
  snprintf(text[rownum], sizeof(text[rownum]), "Sys%01X Gyro%01X Acc%01X Mag%01X", imu_data.cal.system, imu_data.gyro, imu_data.accel, imu_data.mag);
};

void disp::bufferSystem(uint8_t rownum) {  // gtg
  snprintf(text[rownum], sizeof(text[rownum]), "Status%01X Test%01X Err%01X", imu_data.system.status, imu_data.system.self_test_results, imu_data.system.error);
};

void disp::bufferTime(uint8_t rownum) {  // TODO: PLACEHOLDER until we get GPS epoch time
  if (imu_data.orient.tempC != 0) {
    snprintf(text[rownum], sizeof(text[rownum]), "UP: %ul %iC", millis(), imu_data.orient.tempC);
  } else {
    snprintf(text[rownum], sizeof(text[rownum]), "Uptime: %ul --C", millis());
  }
};

void disp::bufferError(thisrow) {  // TODO: INCOMPLETE............................
  snprintf(text[rownum], sizeof(text[rownum]), "ERROR tbd %06.2f", 0.0);
};
