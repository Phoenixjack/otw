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
    Start[Start: Power On] --> Core1Setup[Core 1: Setup Routines]
    Core1Setup -->|Initialize| Serial[Serial Monitor]
    Core1Setup -->|Initialize| GPS[GPS Module]
    Core1Setup -->|Initialize| IMU[IMU Sensor]
    Core1Setup -->|Initialize| SDCard[SD Card Storage]
    Core1Setup -->|Start| Core1Main[Core 1: Primary Main Loop]
    Core1Setup -->|Start| Core2Main[Core 2: Secondary Main Loop]

    Core1Main -->|Run| GPSFunctions[GPS Functions]
    Core1Main -->|Run| IMUFunctions[IMU Data Processing]
    Core1Main -->|Run| SDLogging[Save Data to SD Card]

    Core2Main -->|Run| SerialIO[Serial Monitor I/O]
    Core2Main -->|Run| StatusLED[Status LEDs]
    Core2Main -->|Run| LCDDisplay[LCD Display Updates]
