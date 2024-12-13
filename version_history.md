## V0_0_1:
[X] MASSIVE cleanup/reorg of code
[X] GPS integration
[ ] Integrated PhoenixJack_serializeddatalogger_V2 for writing to SD card

## V0_0_0:
[X] first running version
[X] uses PhoenixJack_serialbuffer_V0 library for non-blocking UART collection
[X] queries BN0055 and all UART ports in main loop
[X] gathers health packet data on UART0 (Serial1) at 115.2K and holds it in buffer array
[X] once packet complete, assembles JSON packet, and includes the latest compass reading
