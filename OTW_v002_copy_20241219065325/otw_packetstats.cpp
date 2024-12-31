








// float vcc; int packnum,gid,uid,tempC,dutrssi,dutnoise,myrssi,mynoise,gps;
// sscanf(rcvbuffer,"%i,%i,%i,%i,%i,%i,%i,%i,%i,%i",packnum,gid,uid,tempC,vcc,dutrssi,dutnoise,myrssi,mynoise,gps);


struct packet_stats {
  uint32_t received_msgs = 0;
  uint32_t last_msg_rcvd = 0;  // millis() timestamp
  bool interval_established = false;
  uint32_t curr_interval_msec = 0;
  uint32_t avg_interval_msec = 0;
  uint32_t expected_earliest = 0;
  uint32_t expected_latest = 0;
  uint16_t missed_packets_consecutive = 0;
  uint16_t missed_packets_total = 0;
  void establish_interval();
  void process_new_msg(uint32_t latest_msg_timestamp);
} packet_stats;

void packet_stats::establish_interval() {  // should we sanity check our curr_interval_msec before we go averaging it in?
  static uint8_t valid_msgs = 1;
  avg_interval_msec *= (valid_msgs);
  avg_interval_msec += curr_interval_msec;
  valid_msgs++;
  avg_interval_msec /= valid_msgs;
  if (valid_msgs > 10) { interval_established = true; }
};
void packet_stats::process_new_msg(uint32_t latest_msg_timestamp) {
  received_msgs++;
  if (last_msg_rcvd > 0) { curr_interval_msec = latest_msg_timestamp - last_msg_rcvd; }
  sprintf(printbuffer, "#msgs: %ul \t LatestTS: %ul  Millis: %ul  Earliest: %ul  Latest: %ul \t MPCons: %u  MPTot: %u \t CurrInt: %ul  AvgInt: %ul", received_msgs, latest_msg_timestamp, millis(), expected_earliest, expected_latest, missed_packets_consecutive, missed_packets_total, curr_interval_msec, avg_interval_msec);
  Serial.println(printbuffer);
  if (interval_established) {
    if (latest_msg_timestamp > expected_earliest && latest_msg_timestamp < expected_latest) {  // check against our established avg_interval_msec (expected_earliest & expected_latest)
      missed_packets_consecutive = 0;
    } else {
      missed_packets_consecutive++;
      missed_packets_total++;
    }
    expected_earliest = millis() + (avg_interval_msec * 0.9);  // update our next expected window
    expected_latest = millis() + (avg_interval_msec * 1.1);
  } else {
    if (curr_interval_msec > 0) { establish_interval(); }
  }
  last_msg_rcvd = latest_msg_timestamp;
};