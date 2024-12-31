# Vehicle mounted data recorder
  - TARGET HARDWARE: RASPBERRY PI PICO W + BOSCH BN0055 + uBLOX NEO-7M
  - AUTHOR: CHRIS MCCREARY
  - VERSION: 0.0.2a
  - DATE: 2024-12-31

#KEY NOTES:
  o For saving to SD card, filenames are limited to 8+3 in length.
  o To keep session names unique, a session is created with a random 16 bit / 2 byte / 4 hex character ID (0x0000-0xFFFF).
  o Session IDs will serve as the prefix for each session.
  o Each session will hold an index file (XXXXindx.txt) and timestamp file (XXXXtime.txt).
    - The index file will hold any header info.
    - The timestamp file will only be created once we have a valid epoch timestamp.
  o Each data file will start with the session ID, and be suffixed with a 16 bit sequence number, for a maximum of 65,535 data files per session.
  o Each data file has an arbitrary limit of 10Kb before it starts a new data file.
  o Assuming an average data entry size of ~160bytes, this should give us:
    - ~6400 entries per data file
    - ~419,424,000 entries per session
  o Assuming a data entry once per second, this should give us:
    - ~6,990,400 minutes / 116,506.6 hours of runtime
  o Assuming an 8Gb SD card, this should give us:
    - ~13 maximum sessions before the disk is full
  o THEORETICALLY! Theoretically, I should be dating Jessica Alba.

# GOALS:
 - Balance CPU usage; currently running on a single core
 - Save CPU time by getting epoch time WITH PPS sync every so often
 - Leverage Mesh networking with onboard WiFi / Dump WiFiManager/MQTT?
 - Over the air message confirmation & repeatback of received remote commands
 - Add packet parsing on the fly, retain count/GID/UID/RSSI/NOISE, dump the rest


## Boot Sequence for Dual-Core Raspberry Pi Pico

```mermaid
flowchart TD
    Start[Start: Power On] --> SetupRoutine[Setup Routine]

    SetupRoutine --> SerialSetup[Setup Serial Monitor]
    SerialSetup --> GPSSetup[Setup GPS Module]
    GPSSetup --> IMUSetup[Setup IMU Sensor]
    IMUSetup --> SDCardSetup[Setup SD Card Storage]
    SDCardSetup --> StartCores[Start Both Cores]

    StartCores --> Core1Main[Core 1: Primary Main Loop]
    StartCores --> Core2Main[Core 2: Secondary Main Loop]

    %% Core 1 Tasks
    Core1Main --> GPSFunctions[Process GPS Functions]
    GPSFunctions --> IMUFunctions[Process IMU Data]
    IMUFunctions --> SDLogging[Log Data to SD Card]
    SDLogging --> Core1Main

    %% Core 2 Tasks
    Core2Main --> SerialIO[Process Serial Monitor I/O]
    SerialIO --> StatusLED[Update Status LEDs]
    StatusLED --> LCDDisplay[Update LCD Display]
    LCDDisplay --> Core2Main
```


## Enhanced Boot Sequence with Shapes and Colors

```mermaid
flowchart TD
    %% Nodes for Setup Routine
    Start[Start: Power On] --> SerialSetup((Setup Serial Monitor))
    SerialSetup --> GPSSetup{{Setup GPS Module}}
    GPSSetup --> IMUSetup([Setup IMU Sensor])
    IMUSetup --> SDCardSetup[Setup SD Card Storage]
    SDCardSetup --> StartCores((Start Both Cores))

    %% Main Loops
    StartCores --> Core1Main((Core 1: Primary Main Loop))
    StartCores --> Core2Main((Core 2: Secondary Main Loop))

    %% Core 1 Tasks
    Core1Main --> GPSFunctions{{Query GPS}}
    Core1Main --> IMUFunctions([Query IMU Sensor])
    Core1Main --> SDLogging[Log Data to SD Card]
    SDLogging --> Core1Main

    %% Core 2 Tasks
    Core2Main --> SerialIO((Receive Serial Input))
    Core2Main --> StatusLED[/Update Status LEDs/]
    Core2Main --> LCDDisplay[\Push Updates to LCD\]
    LCDDisplay --> Core2Main

    %% Styling
    style Start fill:#ffcc00,stroke:#333,stroke-width:2px
    style SerialSetup fill:#99ccff,stroke:#003366,stroke-width:2px
    style GPSSetup fill:#ff9999,stroke:#660000,stroke-width:2px
    style IMUSetup fill:#99ff99,stroke:#006600,stroke-width:2px
    style SDCardSetup fill:#ccccff,stroke:#000099,stroke-width:2px
    style StartCores fill:#ffcc99,stroke:#993300,stroke-width:2px

    style GPSFunctions fill:#ffcccc,stroke:#990000,stroke-width:2px
    style IMUFunctions fill:#ccffcc,stroke:#009900,stroke-width:2px
    style SDLogging fill:#ccccff,stroke:#333399,stroke-width:2px

    style SerialIO fill:#ffffcc,stroke:#999900,stroke-width:2px
    style StatusLED fill:#ffccff,stroke:#993399,stroke-width:2px
    style LCDDisplay fill:#ccffff,stroke:#009999,stroke-width:2px
```