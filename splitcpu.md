
|  CPU1  |  CPU2  |
| ------ | ------ |
| -SETUP- | -SETUP- |
| Min Boot Requirements | Init LCD, WIFI/LORA |
| -> Signal CPU2 | Set runtime settings|
| -LOOP- | -LOOP- |
| GPS update | SerMon input |
| Sensor update | IF resync |
| GPIO update |  > Interrupts/GPS time |
| UART Data Buffer | LCD update |
| IF newdata | Watchdog Routines |
|  > SD write |  > Restart sensor |
|  > Update stats |  > Purge buffer |
| - | WIFI/LORA actions |

```mermaid
    CPU1INIT-->SerMonINIT;
    SerMonINIT-->UARTportINIT;
    UARTportINIT-->GPSportINIT;
    GPSportINIT-->BN0055INIT;
    BN0055INIT-->CPU1loop;
    CPU2INIT-->LCDINIT;
    LCDINIT-->WIFI_LORA_INIT;
    WIFI_LORA_INIT-->CPU2loop;
    BN0055INIT-->CPU1loop;
    CPU1loop-->GPS_UPDATE;
    GPS_UPDATE-->GPIO_UPDATE;
    GPIO_UPDATE-->UART_RX_BUFFER;
    UART_RX_BUFFER-->IF(NEWDATA);
    IF(NEWDATA)-->SD_WRITE;
    SD_WRITE-->PACKET_STATS;
    CPU2loop-->GPS_TIME_UPDATE;
    GPS_TIME_UPDATE-->LCD_UPDATES;
```
