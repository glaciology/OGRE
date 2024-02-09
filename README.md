
<p align="center">
<img src="https://user-images.githubusercontent.com/37055625/181100467-84fbc824-24a9-4d06-a20b-8dd893361870.PNG" width="250"/>
 </p>


# Open GNSS Research Equipment :: A low-power, low-cost GNSS data logger for monitoring the cryosphere.
<p align="center">
<img src="https://github.com/glaciology/OGRENet/assets/37055625/04b9404f-7409-4638-b8b2-81a605f01529" width="350"/>
</p>





## Overview
Originally designed for easily logging multi-GNSS data in remote regions of the Arctic, this instruments incorporates low power, low cost components onto a single circuit board and features a Ublox ZED-F9P GNSS module and Sparkfun Artemis MCU (Ambiq Apollo3 MCU, Cortex-M4). 
- Logs binary (.ubx) GPS, GLONASS, BEIDOU, GALILEO & Satellite Nav Messages to a microSD.
- Nominal data rate under open sky conditions 2000-3000 bytes per solution, equivalent to 5 GB of data for logging all constellations at 15 second interval for a year.
- Nominal current consumption with a 12V supply is 45-65mA (.5-.8W) awake, and 0.07mA (.8mW) asleep.
- Features on-board battery measurement circuit and temperature sensor. 
- Additional pins & peripherals include RX/TX for serial programming, 3-Wire Temperature sensor, secondary I2C bus, secondary UART bus, secondary SPI bus, several GPIO pins and I/O pins for streaming or receiving RTCM messages for RTK operation.  
- Simple 2 layer PCB with SMD components totaling ~$260, including patch antenna. PCB inside enclosure measures 7x6.5x2.5cm. 

<p align="center">
<img src="https://user-images.githubusercontent.com/37055625/207705142-4ba32b05-6b62-4d18-bcf7-4f9eb635589c.jpeg" width="500"/>
</p>

## Project Organization 
[Software: Contains Arduino Code and Test Scripts](Software/)
- [OGRENet: Software and software config files for upload to OGRE instrument](Software/OGRENet) <br>


[Hardware: Hardware & Manufacturing Files](Hardware/)
- [Components: List of PCB parts](Hardware/Components1-3.md)
- [OGRENET Manufacturing Files: PCB Computer Aided Manufacturing Files](Hardware/OGRE_PCB_Gerbers.zip)
- [Hardware Docs: Linked documentation for critical components](Hardware/HardwareDocuments.md)
- [Schematic: Electrical connection schematic](Hardware/OGRE_Schematic1-3.pdf)


## Getting Started 

V2.0.3 of the OGRE has 7 modes of operation: 
  - (1) Daily Fixed Mode: Log GNSS data same time every day, starting & ending during USER-defined start/stop hours, OR
  - (2) Continous Mode: Log GNSS data continously (single file!), OR
  - (3) Monthly Mode: Log GNSS data for 24 hours on a USER-specified day (1-28) each month, OR
  - (4) Interval Mode: Each 24-hour log session is spaced by a USER-defined interval (e.g., log every 3 days for 24 hours), OR
  - (5) Log GNSS data for 24 hours on USER specified dates/times read from a .txt file. Defaults to mode 4 after last user-provided date.
  - (6) Log GNSS data for 24 hours; During winter once on every 10th day (or USER-defined interval); During summer daily. "Summer" is May-August (or USER-defined months). 
  - (99) Test Mode: Used for development. Log GNSS data for 50 second interval, sleep for 50 second and repeat.
  
OUTPUTs: With all modes, GNSS data (phase, doppler, SNR, nav message etc.) are logged to a uSD card in .ubx (UBLOX) proprietary format. Under open sky conditions, we found that an epoch of data (1s) is ~2000-3000 bytes. If logging at 15 seconds for a year, this equates to 6GB of data. A debug file is also generated after each log session is closed, reporting the health of the system (temperature, battery voltage, logging errors, etc.).
  
INPUTs: USERS specify settings in the [CONFIG.txt](OGRENet/CONFIG) file, which, if uploaded to the SD card, will be read into the software. 
Otherwise, software will default to hardcoded configuration. USER may also upload a [EPOCH.txt](OGRENet/EPOCH) file, which allows the user to specify up to 16 log dates (unix epoch format) for logging in Mode 5. 

The CONFIG.TXT file is formatted as follows: 

```
LOG_MODE(1: daily hr, 2: cont, 3: mon, 4: 24 roll, 5: date, 6: season, 99: test)=6
LOG_START_HOUR_UTC(mode 1 only)=17
LOG_END_HOUR_UTC(mode 1 only)=23
LOG_START_DAY(mode 3 only, 0-28)=25
LOG_EPOCH_SLEEP(modes 4/5 only, seconds)=3600
LED_INDICATORS(0-false, 1-true)=1
MEASURE_BATTERY(0-false, 1-true)=0
ENABLE_GPS(0-false, 1-true)=1
ENABLE_GLO(0-false, 1-true)=1
ENABLE_GAL(0-false, 1-true)=1
ENABLE_BDS(0-false, 1-true)=1
ENABLE_QZSS(0-false, 1-true)=0
ENABLE_NAV_SFRBX(0-false, 1-true)=0
STATION_NAME(0000, numeric)=0001
MEASURE_RATE(integer seconds)=1
BAT_SHUTDOWN_V(00.0, volts)=10.9
WINTER_INTERVAL(seconds, mode 6 only)=777600
SUMMER_START_MONTH(mode 6 only)=4
SUMMER_END_MONTH(mode 6 only)=9
end;
```

- If the USER selects LOG_MODE=1, then LOG_START_HOUR_UTC and LOG_END_HOUR_UTC must be specified. 
- If the USER selects LOG_MODE=3, then LOG_START_DAY must also be specified (day of each month GNSS data is logged). 
- If the USER selects LOG_MODE=5, then unix epoch dates for logging are specified in EPOCH.txt. If no dates are specified or if all dates have elapsed, then log interval defaults to LOG_MODE 4, where LOG_EPOCH_SLEEP must be defined.
- If the USER selects LOG_MODE=6, the instrument logs continuously during SUMMER_START_MONTH through SUMMER_END_MONTH (inclusive). Furthermore, the duration between logging during winter is set by WINTER_INTERVAL. Note: log sessions are 24 hours.
- LED_INDICATORS, if false, will disable all LEDs, excluding those present during initialization. 
- MEASURE_BATTERY, if true, battery voltage is measured/monitored, and the instrument will be put to sleep when voltage dips below 10.9V (OR as defined by user in BAT_SHUTDOWN_V). System will restart when voltage measured above ~11.2V (or 0.5V above BAT_SHUTDOWN_V). 
- STATION_NAME is a number between 0001 and 9999, and will be appended to the timestamped file names for each GNSS file.
- MEASURE_RATE is frequency of epoch solutions logged to SD card: 1 = 1 solution per second, 15 = 1 solution per 15 seconds. 

OPERATION:  
Insert the uSD card (with or without CONFIG & EPOCH files), then connect battery to +/- sides of the screw terminal. The system will attempt to initialize and following LED indicators will flash: 
  - 1 Hz Blinks: System acquiring GPS time and attempting to sync real time clock (RTC).
  - 10 rapid Blinks: System Configuration Complete!
  
 The following indicate failure of initialization: 
  - 2 Blink Pattern: uSD initialization failed (is the SD card seated properly?) - system awaiting automatic reset to try again (60 seconds).
  - 3 Blink Pattern: Ublox/antenna initialization failed (is the antenna properly connected?) - system awaiting automatic reset to try again (60 seconds).
  - 5 Blink Pattern: RTC failed to sync with GNSS time (are you outside?) - system awaiting automatic reset to try again (60 seconds). 
  - 6 Rapid Blinks: uSD failed to read CONFIG settings (did you intend to not include the CONFIG file?). 

Once the system is initialized, it will either sleep or begin logging data, depending on the specified log mode. 
If the USER has enabled LED_INDICATORS, the following additional lights will flash: 
  - Flashes (faint) at measurement rate: System logging GNSS data
  - 1 Blink (bright) every 12 seconds: System sleeping
  - No blinks: system is in deep sleep due to low battery, or system is dead due to dead battery.

## Software Upload
A pre-compiled binary is avialable with each release (see releases). This binary file included in the release can be uploaded to the Apollo MCU with a usb-to-serial cable connected to the PCB header pins using the Sparkfun Apollo3 Uploader [here](https://github.com/sparkfun/Apollo3_Uploader_SVL). Example command line prompt using the svl.py script: [use baud -b 115200; provide path to binary file OGRENet.ino.bin; find path of usb serial converter port by typing ls /dev/tty.* on Linux and selecting the proper usb port.] 
```
python3 svl.py -b 115200 -f /PATH/TO/BINARY/FILE/OGRENet.ino.bin /dev/tty.usbserial-####
```

You can also compile the source code with the Arduino IDE, ensuring that the code and board libraries match the proper versions defined in the header of OGRENet.ino [Sparkfun Artemis Module v1.2.3, SDFat library v2.1.0, Sparkfun ublox GNSS library v2.2.8]. 

## Hardware Notes
<p align="center">
<img src="https://user-images.githubusercontent.com/37055625/207705966-89887dd3-3384-4fdd-b19f-6bf830b9bbab.jpg" width="296"/>
<img src="https://user-images.githubusercontent.com/37055625/181101078-56771903-b7e9-467f-81ce-15af4ea97b13.jpg" width="250"/>
</p>

MATERIALS
Cost of PCB and all components totals ~$200. Detailed list of components found [here](Hardware/Components1-3.md). <br>

POWER REQUIREMENTS: 
In standard configuration, this system is powered by a 12V lead-acid battery. 
  
While this system is optimized for 12V batteries, input voltage can range from 5.2V to 20V with the following considerations/customizations:  
  - The DC-DC converter minimum input is 5.2V and maximum input is 50V, although aditional power filtering at high voltages is required. 
  - The Battery Measurement circuit features a voltage divider circuit that must scale max voltage to 3.3V for the ADC pin. Standard dividers for a 12V battery use 68kOhm and 10kOhm resistors. USER must adjust gain/offset of ADC battery measurement conversion in software if using different power configuration (i.e., different resistor dividers and/or a non-12V battery). 
  - The reverse polarity protection MOSFET has a limit of 20V. **Do not exeed 20V** without either removing this part or finding an appropriate substitute component. 

## License & Credits
This project is open source! OGRENet software is released under the [MIT License](http://opensource.org/licenses/MIT).

Portions of this code for this project are derived from [Sparkfun GNSS Library](https://github.com/sparkfun/SparkFun_u-blox_GNSS_Arduino_Library) [Sparkfun Arduino Core](https://github.com/sparkfun/Arduino_Apollo3/releases/tag/v1.2.0), [Sparkfun OpenLog GNSS](https://github.com/sparkfun/OpenLog_Artemis_GNSS_Logger), and
[Cryologger Glacier Velocity Tracker v2.0.3](https://github.com/adamgarbo/Cryologger_Glacier_Velocity_Tracker).

Modifications Copyright (c) 2020 SparkFun
Modifications Copyright (c) 2020 Adam Garbo

See [License](LICENSE.md) for more details.
