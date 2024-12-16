
|  CPU1  |  CPU2  |
| ------ | ------ |
| -SETUP- | -SETUP- |
| Min Boot Requirements | Init LCD, LORA |
| -> Signal CPU2 | Set runtime settings|
| -LOOP- | -LOOP- |
| GPS update | SerMon input |
| Sensor update | GPIO update |
| UART Data Buffer | LCD update |
| IF newdata | -  |
|  > SD write | - |