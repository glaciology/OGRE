
<p align="center">
<img src="https://user-images.githubusercontent.com/37055625/181100467-84fbc824-24a9-4d06-a20b-8dd893361870.PNG" width="250"/>
 </p>


# On-ice Greenland Research Experimental Network :: A low-power, low-cost GNSS data logger "OGRE" for monitoring the cryosphere.
<p align="center">
<img src="https://user-images.githubusercontent.com/37055625/181101164-2deffa3e-e22e-488c-9098-c6bd66c3908c.jpg" width="350"/>
</p>

## Overview
Designed for logging raw multi-GNSS data in remote regions of the Arctic, this instruments incorporates low power, low cost components onto a single circuit board and features a Ublox ZED-F9P/T GNSS module and Sparkfun Artemis MCU (Ambiq Apollo3 MCU, Cortex-M4). 
- Logs raw/binary GPS, GLONASS, BEIDOU, GALILEO, QZSS & Satellite Nav Messages at 1 Hz to microSD.
- Nominal current consumption with a 12V supply is 45-65mA (.54W) awake, and 0.07mA (.84mW) asleep.
- Features on-board battery measurement circuit and temperature sensor. 
- Additional pins & peripherals include RX/TX for serial programming, 3-Wire Temperature sensor, secondary I2C bus, secondary UART bus, secondary SPI bus, several GPIO pins and I/O pins for streaming or receiving RTCM messages for RTK operation.  
- Simple 2 layer PCB with SMD components totaling ~$260, including patch antenna. PCB inside enclosure measures 7x6.5x2.5cm. 

<p align="center">
<img src="https://user-images.githubusercontent.com/37055625/207705142-4ba32b05-6b62-4d18-bcf7-4f9eb635589c.jpeg" width="500"/>
</p>

## Project Organization 
[/Software: Contains Arduino Code and Test Scripts](Software)
- [OGRENet: Software and software config files for upload to OGRE instrument](OGRENet) <br>


[/Hardware: Hardware & manufacturing files](Hardware)
- [Components.md: List of PCB parts](Hardware/Components.md)
- [OGRENET_3/22_Manufacturing_Files: PCB Computer Aided Manufacturing Files](Hardware/OGRENET_3/22_Manufacturing_Files.zip)
- [Hardware Docs: Documentation for critical components](Hardware/HardwareDocs)
- [Schematic: Electrical connection schematic](Hardware/GNSS_Schematic2.pdf)


## Getting Started 

The Software has 6 modes of operation: 
  - (1) Daily Fixed Mode: Log GNSS data same time every day, starting & ending during USER defined hours
  - (2) Continous Mode: Log GNSS data continously
  - (3) Monthly Mode: Log GNSS data for 24 hours on a USER specified day each month
  - (4) Sleep Interval Mode: Log GNSS data for 24 hours at power-on and at a USER defined interval thereafter
  - (5) Log GNSS data for 24 hours on USER specified dates/times read from .txt file. Defaults to mode 4 after last user provided date.
  - (99) Test Mode: Log GNSS data for 50 second interval, sleep for 50 second and repeat.
  
OUTPUTs: With all modes, GNSS data (phase, doppler, SNR, etc.) is logged to a uSD card in raw .ubx (UBLOX) proprietary format. A debug file is also generated after each log session is closed, reporting the health of the system (temperature, battery health, logging errors, etc.).
  
INPUTs: USERS specify settings in the [CONFIG.txt](OGRENet/CONFIG) file, which, if uploaded to the SD card, will be read into the software. 
Otherwise, software will default to hardcoded configuration. USER may also upload a [EPOCH.txt](OGRENet/EPOCH) file, which allows the user to specify up to 16 log dates (unix epoch format) for logging in Mode 5. 

The CONFIG.TXT file is formatted as follows: 

```
LOG_MODE(1: daily fixed, 2: cont, 3: mon, 4: 24 roll, 5: date, 99: test)=5
LOG_START_HOUR_UTC(only if using mode 1)=12
LOG_END_HOUR_UTC(only if using mode 1)=14
LOG_START_DAY(only if using mode 3, 0-28)=25
LOG_EPOCH_SLEEP(only if using mode 4/5, seconds)=1800
LED_INDICATORS(0-false, 1-true)=1
MEASURE_BATTERY(0-false, 1-true)=1
ENABLE_GPS(0-false, 1-true)=1
ENABLE_GLO(0-false, 1-true)=1
ENABLE_GAL(0-false, 1-true)=1
ENABLE_BDS(0-false, 1-true)=1
ENABLE_QZSS(0-false, 1-true)=0
ENABLE_NAV_SFRBX((0-false, 1-true)=1
STATION_NAME(0000)=0001
break;
```

- If the USER selects LOG_MODE=1, then LOG_START_HOUR_UTC and LOG_END_HOUR_UTC must be specified. 
- If the USER selects LOG_MODE=3, then LOG_START_DAY must also be specified (day of each month GNSS data is logged). 
- If the USER selects LOG_MODE=5, then unix epoch dates for logging are specified in EPOCH.txt. If no dates are specified or if all dates have elapsed, then log interval defaults to LOG_MODE 4, where LOG_EPOCH_SLEEP must be defined. 
- LED_INDICATORS, if false, will disable all LEDs for power savings, excluding those present during initialization. 
- MEASURE_BATTERY can be enabled if the user has installed the 12V Lead Acid Battery Circuit and desires battery voltage measurements included in the DEBUG file. This feature will also activate a function that checks the health of the battery and put the instrument to sleep when voltage dips below 10.8V. System will restart when voltage measured above ~11.2V. 
- STATION_NAME is a number between 0001 and 9999, and will be appended to the timestamped file names for each GNSS file. 

OPERATION:  
Insert the uSD card (with or without CONFIG & EPOCH files), then connect battery. The system will attempt to initialize and following LED indicators will flash: 
  - 1 Hz Blinks: System acquiring GPS time and attempting to sync real time clock (RTC).
  - 10 rapid Blinks: System Configuration Complete!
  
 The following indicate failure of initialization: 
  - 2 Blink Pattern: uSD initialization failed - system awaiting automatic reset to try again (60 seconds).
  - 3 Blink Pattern: Ublox initialization failed - system awaiting automatic reset to try again (60 seconds).
  - 5 Blink Pattern: RTC failed to sync with GNSS time - system awaiting automatic reset to try again (60 seconds). 
  - 5 Rapid Blinks: uSD failed to read CONFIG settings. 

Once the system is initialized, it will either sleep or begin logging data, depending on the specified log mode. 
If the USER has enabled LED_INDICATORS, the following additional lights will flash: 
  - Random rapid flashes: System logging GNSS data
  - 1 Blink every 12 seconds: System sleeping

## Installation
A pre-compiled binary is avialable with each release. V1.0.6 [here](https://github.com/glaciology/OGRENet/releases/tag/v1.0.6). This can be uploaded to the Apollo MCU with a usb-serial cable connected to the PCB header pins using the Sparkfun Apollo3 Uploader [here](https://github.com/sparkfun/Apollo3_Uploader_SVL). 

You can also compile this code with the Arduino IDE, ensuring that the code and board libraries match the proper versions defined in the header of OGRENet.ino. 

## Hardware Notes
<p align="center">
<img src="https://user-images.githubusercontent.com/37055625/207705966-89887dd3-3384-4fdd-b19f-6bf830b9bbab.jpg" width="296"/>
<img src="https://user-images.githubusercontent.com/37055625/181101078-56771903-b7e9-467f-81ce-15af4ea97b13.jpg" width="250"/>
</p>

MATERIALS
Cost of PCB and all components totals ~$260. Detailed list of components found [here](Hardware/Components.md). <br>

POWER REQUIREMENTS: 
In standard configuration, this system is powered by a 12V lead-acid battery. 
  
While this system is optimized for 12V batteries, input voltage can range from 5.2V to 20V with the following considerations:  
  - The Pololu DC-DC converter minimum input is 5.2V and maximum input is 50V, although aditional power filtering at high voltages required. 
  - The Battery Measurement circuit features a voltage divider circuit that must scale max voltage to 3.3V for the ADC pin. Standard dividers for a 12V battery use 68kOhm and 10kOhm resistors. USER must adjust gain/offset of ADC battery measurement conversion in software if using different power configuration (i.e., different resistor dividers and/or different battery voltage). 
  - The reverse polarity protection MOSFET has a limit of 20V. **Do not exeed 20V** without either removing this part or finding an appropriate substitute component. 

## License & Credits
This project is open source! OGRENet software is released under the [MIT License](http://opensource.org/licenses/MIT).

Some [code](Software/TestCode) for this project is adapted from Sparkfun, released under the same license. 
