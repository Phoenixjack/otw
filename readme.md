# Vehicle mounted data recorder
  - TARGET HARDWARE: RASPBERRY PI PICO W + BOSCH BN0055 + uBLOX NEO-7M
  - AUTHOR: CHRIS MCCREARY
  - VERSION: 0.0.2a
  - DATE: 2024-12-31

# KEY NOTES:
  - For saving to SD card, filenames are limited to 8+3 in length.
  - To keep session names unique, a session is created with a random 16 bit / 2 byte / 4 hex character ID (0x0000-0xFFFF).
  - Session IDs will serve as the prefix for each session.
  - Each session will hold an index file (XXXXindx.txt) and timestamp file (XXXXtime.txt).
    - The index file will hold any header info.
    - The timestamp file will only be created once we have a valid epoch timestamp.
  - Each data file will start with the session ID, and be suffixed with a 16 bit sequence number, for a maximum of 65,535 data files per session.
  - Each data file has an arbitrary limit of 10Kb before it starts a new data file.
  - Assuming an average data entry size of ~160bytes, this should give us:
    - ~6400 entries per data file
    - ~419,424,000 entries per session
  - Assuming a data entry once per second, this should give us:
    - ~6,990,400 minutes / 116,506.6 hours of runtime
  - Assuming an 8Gb SD card, this should give us:
    - ~13 maximum sessions before the disk is full
  - THEORETICALLY! Theoretically, I should be dating Jessica Alba.

# GOALS:
 - Balance CPU usage; currently running on a single core
 - Save CPU time by getting epoch time WITH PPS sync every so often
 - Leverage Mesh networking with onboard WiFi / Dump WiFiManager/MQTT?
 - Over the air message confirmation & repeatback of received remote commands
 - Add packet parsing on the fly, retain count/GID/UID/RSSI/NOISE, dump the rest


## Enhanced Boot Sequence with Shapes and Colors

```mermaid
flowchart TD
    %% Define Nodes
    Start([Start]) --> InitSerMon{{Init Serial Mon}}
    InitSerMon --> InitUARTin{{Init UART in}}
    InitUARTin --> InitGPS{{Init GPS}}
    InitGPS --> InitIMU{{Init IMU}}
    InitIMU --> InitSDcard{{Init SD card}}
    InitSDcard --> InitLCD{{Init LCD}}
	InitLCD --> MinBootReqs{"Min Boot Requirements Met?"}
	MinBootReqs --> |YES| StartBothCores[Start Both Cores]
    MinBootReqs --> |NO| InitSerMon{{Init Serial Mon}}
	
    %% Main Loops
    StartBothCores --> Core1Main[Core 1: Main Loop]
    StartBothCores --> Core2Main[Core 2: Main Loop]

    %% Core 1 Tasks
    Core1Main --> GPSFunctions[/Receive GPS/]
    GPSFunctions --> IMUFunctions([Query IMU Sensor])
	IMUFunctions --> GetUARTdata[/Receive UART data/]
    GetUARTdata --> SDLogging[\Log Data to SD Card\]
    SDLogging --> Core1Main

    %% Core 2 Tasks
    Core2Main --> SerialIO[/Receive Serial Input/]
    SerialIO --> StatusLED[\Update Status LEDs\]
    StatusLED --> LCDDisplay[\Push Updates to LCD\]
    LCDDisplay --> Core2Main

    %% SUBSYSTEMS
    InitSerMon --> StartSerMon
	StartSerMon([Init SerMon]) --> InitSerial{{Serial:115200 8N1}}
	InitSerial --> ReportSerMon([Report SerMon Status])
	ReportSerMon --> WatchdogInit[Watchdog Init]
	
    InitUARTin --> StartUARTIn
    StartUARTIn([Init SerMon]) --> InitUARTIn{{Serial1:115200 8N1}}
	InitUARTIn --> ReportUARTIn([Report SerMon Status])
	ReportUARTIn --> WatchdogInit

    InitGPS --> StartGPS
    StartGPS([Init GPS]) --> InitSerial2{{Serial2:9600 8N1}}
	InitSerial2 --> ListenGPS[/"Listen GPS:1.5secs"/]
	ListenGPS --> ReportGPS([Report GPS Status])
	ReportGPS --> WatchdogInit

    InitIMU --> StartIMU
    StartIMU([Init IMU]) --> ConfigIMU{{"IMU:OPERATION_MODE_NDOF"}}
	ConfigIMU --> SelfTestIMU[/"IMU Self Test"/]
	SelfTestIMU --> ReportIMU([Report IMU Status])
	ReportIMU --> WatchdogInit

    InitSDcard --> StartSD([Init SD])
    StartSD --> ConfigSD{{"SPI Init: RP2040 SD Library"}}
	ConfigSD --> ReportSD([Report SD Status])
	ReportSD --> WatchdogInit

    InitLCD --> StartLCD([Init LCD])
    StartLCD([Init IMU]) --> ConfigLCD{{"I2C Init: PCF8574 Iface"}}
	ConfigLCD --> ReportLCD([Report LCD Status])
	ReportLCD --> WatchdogInit

	WatchdogInit --> MinBootReqs

    %% Define Classes
    classDef startStop fill:#4CAF50,stroke:#333,stroke-width:2px,font-size:14px,color:#fff;
    classDef process fill:#2196F3,stroke:#333,stroke-width:2px,font-size:14px,color:#fff;
    classDef decision fill:#FF9800,stroke:#333,stroke-width:2px,font-size:14px,color:#fff;
    classDef input fill:#9C27B0,stroke:#333,stroke-width:2px,font-size:14px,color:#fff;
    classDef output fill:#9C27B0,stroke:#333,stroke-width:2px,font-size:14px,color:#fff;
    classDef config fill:#0CC0DF,stroke:#333,stroke-width:2px,font-size:14px,color:#fff;

    %% Assign Classes
    class Start,End,StartSerMon,ReportSerMon,StartUARTIn,ReportUARTIn,StartGPS,ReportGPS,StartIMU,ReportIMU startStop;
    class StartSD,StartLCD,ReportLCD,ReportSD startStop
    class WatchdogInit,Core1Main,Core2Main,StartBothCores process;
    class MinBootReqs decision;
    class GPSFunctions,SerialIO,GetUARTdata,ListenGPS,SelfTestIMU input;
	class SDLogging,StatusLED,LCDDisplay output;
	class InitSerMon,InitUARTin,InitUARTIn,InitGPS,InitIMU,InitSDcard,InitLCD,InitSerial,InitSerial2,ConfigIMU config;
    class ConfigLCD,ConfigSD config
```

https://mermaid.live/

https://www.mermaidflow.app/editor

https://mermaid.js.org/intro/

