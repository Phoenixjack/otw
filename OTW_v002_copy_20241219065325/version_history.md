## V0_0_2:
[ ] fix packet stats code
[ ] rename additional files from bn0055_xxx.h to otw_xxx.cpp
[ ] have GPS struct save data to itself and REPORT struct pull from GPS
[ ] introduce lcd display
[ ] fixed double entry of first filename


## V0_0_1:
[X] first running version that logs GPS data to SD card
[X] MASSIVE cleanup/reorg of code
[X] GPS integration
[X] Integrated PhoenixJack_serializeddatalogger_V2 for writing to SD card 
[X] ~~Modified main loop to log GPS/heading every second, regardless of whether we have data missed_packets_consecutive~~
[X] ~~Modified main loop to replace report.buffer with "NULL" after handling a health packet. Prevents us from writing the last packet repeatedly after packets drop~~
BUGS:
  o File Write LED comes on when writing to file, but never blinks or turns off
  o Once missed packet fault comes up, it never clears. Fault LED comes on, but doesn't clear
  o ~~The first CSV filename is listed in the index file twice~~
  o SD datalogger works fine in standalone mode, but when embedded here, doesn't create the timefile.
  o GPS invalidation isn't kicking in. Should drop after xx seconds without a new fix

## V0_0_0:
[X] first running version that integrates sensor and packet data
[X] uses PhoenixJack_serialbuffer_V0 library for non-blocking UART collection
[X] queries BN0055 and all UART ports in main loop
[X] gathers health packet data on UART0 (Serial1) at 115.2K and holds it in buffer array
[X] once packet complete, assembles JSON packet, and includes the latest compass reading
