# OGRENet
On-ice Greenland Research Experimental Network

## Introduction/Hardware Overview
Designed for logging raw GNSS data in remote regions of the Arctic, this custom PCB features a Ublox ZED-F9P and Sparkfun Artemis Module (featuring Ambiq Apollo3 MCU, Cortex-M4) which communicates with the GNSS module over I2C, with data logged to a uSD card. Input power is from a 12V lead acid batteries, although any power supply in the 6V-25V range is acceptable. Nominal current consumption with a 12V supply is 45mA (tracking GPS and GLONASS constellations and logging 1Hz data), while consuming 0.07mA during sleep. An on board battery voltage measurement circuit and temperature sensor provide corresponding readings for each GNSS data logging session. Additional optional peripherals and pins include RX/TX for serial programming, 3-Wire Temperature sensor, an additional I2C bus for peripheral communication and several GPIO pins that can be configured as ADC, UART, etc. 

<img src="https://user-images.githubusercontent.com/37055625/156889824-d95c58e6-8be3-41f7-8ab4-701e75ca4c8e.jpg" width="350"/> <img src="Hardware/Silkscreen.png" width="600"/>

**NOTE: DO NOT power board above 25V. Battery measurements are made with a voltage divider that must scale max voltage to 3.3V for ADC. V1.0 dividers use 68kOhm and 10kOhm resistors, limiting max power to 25V. If higher power desired, change voltage divider resistors.**

## How to Use This Software

This Software has 4 modes of operation: 
  - (1) Daily Fixed Mode: Log GNSS data every day, starting/ending during USER defined hours
  - (2) Continous Mode: Log GNSS data continously
  - (3) Monthly Mode: Log GNSS data for 24 hours on a specified day each month
  - (4) Test Mode: Log GNSS data for 50 second intervals, sleep for 50 second intervals
  
OUTPUTs: With all modes, GNSS data is logged to a uSD card in raw .ubx (UBLOX) proprietary format. A debug file is also generated, reporting the health of the system (temperature, errors, etc.) after each log session is closed.
  
INPUTs: USERS specify settings in the [CONFIG.txt](OGRENet/CONFIG) file, which, if uploaded to the SD card, will be read into the software. 
Otherwise, software will default to hardcoded values **SPECIFY HERE**

The CONFIG.TXT file is formatted in the following way: 

```
LOG_MODE(1: daily fixed, 2: continuous, 3: monthly, 4: test)=2
LOG_START_HOUR_UTC(only if using mode 1)=15
LOG_END_HOUR_UTC(only if using mode 1)=18
LOG_START_DAY(only if using mode 4, 0-28)=5
LED_INDICATORS(0-false, 1-true)=1
MEASURE_BATTERY(0-false, 1-true)=0
```

In addition to specifying LOG_MODE, if the USER selects LOG_MODE=1, then LOG_START_HOUR_UTC and LOG_END_HOUR_UTC must be specified. 
If the USER selects LOG_MODE=3, then LOG_START_DAY must also be specified (day of each month GNSS data is logged). 
LED_INDICATORS, if false, will disable all LEDs, excluding those present during initialization. 
MEASURE_BATTERY can be enabled if the user has installed the Battery Circuit and desires battery voltage measurements included in the DEBUG file. 

OPERATION:  
After plugging in the PCB and inserting the uSD card, the system will attempt to initialize and following LED indicators will flash: 
  - 1 Hz Blinks: System acquiring GPS time and attempting to sync real time clock (Modes 1 and 3 ONLY).
  - 10 final Blinks: System Configuration Complete!
  
 The following indicate failure of initialization: 
  - 2 Blink Pattern: uSD failed - system awaiting reset.
  - 3 Blink Pattern: Ublox failed - system awaiting reset.

Once the system is initialized, it will either sleep or begin logging data, depending on the mode. 
If the USER has enabled LED_INDICATORS, the following additional lights will flash: 
  - Random flashes: System logging GNSS data
  - 1 Blink every 10 seconds: System sleeping


## Design
## Materials
## License & Credits
This project is released under the [MIT License](http://opensource.org/licenses/MIT).

Some code for this project adapted from [Sparkfun GNSS Library](https://github.com/sparkfun/SparkFun_u-blox_GNSS_Arduino_Library) and 
[Cryologger Glacier Velocity Tracker](https://github.com/adamgarbo/Cryologger_Glacier_Velocity_Tracker)
