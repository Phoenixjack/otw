
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
graph TD;
    A-->B;
    A-->C;
    B-->D;
    C-->D;
```



'''mermaid
sequenceDiagram
A->> B: Query
B->> C: Forward query
Note right of C: Thinking...
C->> B: Response
B->> A: Forward response
'''