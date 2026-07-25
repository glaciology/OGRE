
<p align="center">
<img src="https://github.com/glaciology/OGRE/assets/37055625/38e84161-e7fa-485c-b498-54799fec6b80" width="250"/>
 <img src="https://user-images.githubusercontent.com/37055625/181100467-84fbc824-24a9-4d06-a20b-8dd893361870.PNG" width="250"/>
 </p>


# Open GNSS Research Equipment :: A low-power, low-cost GNSS data logger for monitoring the cryosphere.
<p align="center">
<img src="https://github.com/user-attachments/assets/352f2a3b-dfde-4013-85c5-045853ee4342" width="500"/>
</p>






## Overview
Originally designed for easily logging multi-GNSS data in remote regions of the Arctic, this instruments incorporates low-power, low-cost components onto a single circuit board and features a Ublox ZED-F9P/X20P GNSS module and Ambiq Apollo3 MCU. By stripping away many of the features of commercial options, we streamline this instrument specifically for "set it and forget it" rapid deployments, acquiring high quality GNSS data.
- Logs GPS, GLONASS, BEIDOU, GALILEO & Satellite Nav Messages to a microSD, which can be easily converted to standard RINEX format (from .ubx) for positioning with PPK or PPP (RTK is possible but DIY). F9P variants (hardware v1.0) log L1/L2, X20P variant logs L1/L2/L5 (hardware v2.0). 
- Easily configurable via microSD card. Many configuration options to optimize data and battery needs, such as a polar-specific mode of daily logging during summertime and weekly logging during the winter, or a sunrise/sunset mode for logging data for 3 hours, twice daily to capture diurnal glacial signals. 
- Low cost, totaling ~$175, not including external components. PCB inside enclosure measures 7x6.5x2.5cm. 
- Current consumption with a 12V supply is 45-65mA (.5-.8W) awake, and 0.07mA (.8mW) asleep. We have used a single 12V 40Amp hr battery with a 10W solar panel to log continuously for 3 hours a day every day, including throughout the long polar night in Greenland.
- Programmed in Arduino environment, for easy customization.

A detailed guide is maintained [here](https://docs.google.com/document/d/1q8-jAFY2cO8MIOHwoCQhJwPjmURbkkGEsrDBrOs5gMM/edit?usp=sharing). 

<p align="center">
<img src="https://user-images.githubusercontent.com/37055625/207705142-4ba32b05-6b62-4d18-bcf7-4f9eb635589c.jpeg" width="500"/>
</p>

## Project Organization 
[Software: Contains Arduino Code and Test Scripts](Software/)
- [OGRE: Software and software config files for upload to OGRE instrument](Software/OGRE) <br>
[Hardware: Hardware & Manufacturing Files](Hardware/)
- Here you will find the component lists for OGREv1 and OGREv2. We recommend all future acquisitions use OGREv2, which is optimized for the X20P reciever. However, use v1 if you or your manufacturer cannot flash the device using a SWD/JTAG interface. See the firmware section for details on flashing the correct firmware onto your v1 or v2 device.  

Again, a detailed, up-to-date guide can found [here](https://docs.google.com/document/d/1q8-jAFY2cO8MIOHwoCQhJwPjmURbkkGEsrDBrOs5gMM/edit?usp=sharing)!

## Getting Started 

The OGRE has the following modes of operation (note, not all modes are available on old versions of the firmware, upgrade may be necessary): 
  - (1) Daily Fixed Mode: Log GNSS data same time every day, starting & ending during USER-defined start/stop hours, OR
  - (2) Continous Mode: Log GNSS data continously (new file generated at each midnight UTC), OR
  - (3) Monthly Mode: Log GNSS data for 24 hours on a USER-specified day (1-28) each month, OR
  - (4) Interval Mode: Each 24-hour log session is spaced by a USER-defined interval (e.g., log every 3 days for 24 hours), OR
  - (5) Log GNSS data for 24 hours on USER specified dates/times read from a .txt file. Defaults to mode 4 after last user-provided date.
  - (6) Log GNSS data for 24 hours; During winter once on every Nth day (or USER-defined interval, in # of days); During summer daily. "Summer" is May-August (or USER-defined months).
  - (7) Reserved.
  - (8) Log GNSS data twice daily for USER specified duration, during-USER defined morning period and USER-defined evening. 
  - (99) Test Mode: Used for development. Log GNSS data for 50 second interval, sleep for 50 second and repeat.
  
OUTPUTs: With all modes, GNSS data (phase, doppler, SNR, nav message etc.) are logged to a uSD card in .ubx (UBLOX) proprietary format. Under open sky conditions, we found that an epoch of data (1s) is ~2000-3000 bytes. If logging at 15 seconds for a year, this equates to 6GB of data. A debug file is also generated after each log session is closed, reporting the health of the system (temperature, battery voltage, logging errors, etc.). 
  
INPUTs: USERS specify settings in the CONFIG.txt file, which, if uploaded to the SD card, will be read into the software. If no CONFIG.txt exists on the attached SD card, it will automatically be added to the SD card with the default settings upon initial power-on.
Otherwise, software will default to hardcoded configuration. USER may also upload a EPOCH.txt file, which allows the user to specify up to 20 log dates (unix epoch format) for logging in Mode 5. The CONFIG.txt and EPOCH.txt files are Windows and Mac (e.g., Notepad or Textedit) compatable (previously, carriage return characters \n caused issues for Windows-generated files). Using an older CONFIG.txt version will not crash the OGRE, but will cause the OGRE to use the default settings instead. 

The CONFIG.TXT file is formatted as follows (make sure to use the right version, or let the device auto-populate the template onto your SD card): 

```
LOG_MODE(1: daily hr, 2: cont, 3: mon, 4: 24 roll, 5: date, 6: season, 99: test)=2
LOG_START_HOUR_UTC(mode 1 only)=17
LOG_END_HOUR_UTC(mode 1 only)=23
LOG_START_DAY(mode 3 only, 0-28)=25
LOG_EPOCH_SLEEP(modes 4/5 only, seconds)=3600
LED_INDICATORS(0-false, 1-true)=1
MEASURE_BATTERY(0-false, 1-true)=0
ENABLE_GPS(0-false, 1-true)=1
ENABLE_GLO(0-false, 1-true)=1
ENABLE_GAL(0-false, 1-true)=1
...
...
...
...
```

- If the USER selects LOG_MODE=1, then LOG_START_HOUR_UTC and LOG_END_HOUR_UTC must be specified. 
- If the USER selects LOG_MODE=3, then LOG_START_DAY must also be specified (day of each month GNSS data is logged). 
- If the USER selects LOG_MODE=5, then unix epoch dates for logging are specified in EPOCH.txt. If no dates are specified or if all dates have elapsed, then log interval defaults to LOG_MODE 4, where LOG_EPOCH_SLEEP must be defined.
- If the USER selects LOG_MODE=6, the instrument logs continuously during SUMMER_START_MONTH + SUMMER_START_DAY through SUMMER_END_MONTH + SUMMER_END_DAY (inclusive). Furthermore, the duration between logging during winter is set by WINTER_INTERVAL. Note: log sessions are 24 hours.
- If the USER selects LOG_MODE=8, then LOG_START_HOUR_UTC and LOG_END_HOUR_UTC must be specified, along with LOG_START_HR_TWO and LOG_END_HR_TWO. 
- LED_INDICATORS, if false, will disable all LEDs, excluding those present during initialization. 
- MEASURE_BATTERY, if true, battery voltage is measured/monitored, and the instrument will be put to sleep when voltage dips below 10.9V (OR as defined by user in BAT_SHUTDOWN_V). System will restart when voltage measured above ~11.2V (or 0.5V above BAT_SHUTDOWN_V). 
- STATION_NAME is a number between 0000 and zzzz (no special characters, please!), and will be appended to the timestamped file names for each GNSS file.
- MEASURE_RATE is frequency of epoch solutions logged to SD card: 1 = 1 solution per second, 15 = 1 solution per 15 seconds. Range is .5 to 60 (seconds). 

BASIC OPERATION:  
Insert the uSD card (with or without CONFIG & EPOCH files), then connect battery to +/- sides of the screw terminal. The system will attempt to initialize and following LED indicators will flash: 
  - 1 Hz Blinks: System acquiring GPS time and attempting to sync real time clock (RTC).
  - 10 rapid Blinks: System Configuration Complete!
  
 The following LED patterns indicate success or failure for OGREv2. For OGREv1, there is only 1 led. 
| Pattern | Meaning | Color (v2) | Triggered From |
|---|---|---|---|
| 2 blink pattern, then waits for reset | microSD card failed to initialize (twice) | 🔴 Red (`LED`) | `configureSD()` |
| 3 blink pattern, then waits for reset | u-blox I2C not detected, or GNSS constellation config failed | 🔴 Red (`LED`) | `configureGNSS()` |
| 5 blink pattern, then waits for reset | RTC failed to sync with GNSS within 3-minute window | 🔴 Red (`LED`) | `syncRtc()` |
| 6 rapid blinks | `CONFIG.TXT` missing/unreadable — using hard-coded defaults | 🔴 Red (`LED`) | `getConfig()` |
| 10 blinks | RTC synced + system configuration complete (boot only) | 🟡 Yellow (`LED2`) | `setup()` |
| 1 blink, roughly every 12 seconds | Device sleeping between sessions | 🔴 Red (`LED`) | `loop()` |
| 1 Hz blink | Acquiring GNSS fix during RTC sync | 🟡 Yellow (`LED2`) | `syncRtc()` |
| Rapid, irregular blinks while active, approx. corresponding to logging rate | Actively logging GNSS data to SD | 🔴 Red (`LED`) | `logGNSS()` |
| No blinks at all | Deep sleep — battery below shutdown threshold | *(none — both LEDs off)* | `checkBattery()` / `goToSleep()` |

## Software Upload
For OGREv1, software is uploaded using a USB-to-Serial converter. 
For OGREv2, software is uploaded using JTAG/SWD, or, if the proper bootloader is installed on the MCU, can be done over USB-to-Serial.

Only do this if you want to update the firmware on the OGRE, or if the OGRE has not yet had the firmware installed. A pre-compiled binary file is available with each release (see [releases](https://github.com/glaciology/OGRE/releases/)). Note: there are separate binaries for v1 and v2 OGREs. This binary file included in the release can be uploaded to the Apollo MCU (OGREv1) with a usb-to-serial cable connected to the PCB header pins using the Sparkfun Apollo3 Uploader [here](https://github.com/sparkfun/Apollo3_Uploader_SVL). 

For v2 OGREs, please reach out to me if you have questions that come up. The HARDWARE_VERSION macro must be set properly when manually compiling the code.

### OGRE V1
The USB to Serial converter is attached to the OGRE via the 5 through-hole pins on the PCB: attach Ground to GND, RX -> TX, TX->RX, etc. NOTE: the serial converter must be 3.3V. DO NOT EXPOSE pins to 5V. 

Example command line prompt using the svl.py script: [use baud -b 115200; provide path to binary file OGRE.ino.bin; find path of usb serial converter port by typing ls /dev/tty.* on Linux and selecting the proper usb port.] 
```
python3 svl.py -b 115200 -f /PATH/TO/BINARY/FILE/OGRE.ino.bin /dev/tty.usbserial-####
```

You can also compile the source code with the Arduino IDE, ensuring that the code and board libraries match the proper versions defined in the header of OGRE.ino [Sparkfun Artemis Module v1.2.3, SDFat library v2.1.0, Sparkfun ublox GNSS library v2.2.8]. 

### OGRE V2
If the bootloader is installed on OGRE v2, you can follow the above steps (same as OGRE v1). Otherwise, or if you don't know what I mean by this, you'll have to use JTAG/SWD. More details to come...

## Hardware Notes
<p align="center">
<img src="https://github.com/user-attachments/assets/226c09d0-3a6c-419c-b80c-52f5dac20911" width="250"/>
</p>

MATERIALS
Cost of PCB and all components totals ~$175. Detailed list of components found in the Hardware folder. <br>

ASSEMBLY
Assembly services are available through PCBWay, and the OGRE can be ordered directly from [here](https://www.pcbway.com/project/shareproject/OGRE_Open_GNSS_Research_Equipment_Receiver_33a809f3.html) for V1 or from [here](https://www.pcbway.com/project/shareproject/OGRE_Open_GNSS_Research_Equipment_v2_0_82247d12.html) for V2. Note that this assembly service often does not include the [Pololu](https://www.pololu.com/product/3792) component, which must be soldered (4 through-hole) by the user (requests may be made to PCBWay to see if they can source this component, or the user can ship to PCBWay). The user must also program the device following the instructions from the Software Upload section. OGRE2.0 (with ublox X20P) can similarly be ordered from [here](https://www.pcbway.com/project/shareproject/OGRE_Open_GNSS_Research_Equipment_v2_0_82247d12.html), and we highly recommend the customer asks PCBWay to upload the .bin firmware to the board using their JTAG/SWD interface. Instructions for PCBWay upload can be found [in Upload_Instructions](/Hardware/OGRE_v2.0/). 

If you choose to use a u-blox antenna, you will need a ground plane, which you can make out of any metal disc, or you can order pre-drilled discs from PCBWay too, [here](https://www.pcbway.com/project/shareproject/Ground_Plane_for_ublox_L1_L2_L5_GNSS_antenna_c3519487.html), compatitble for both L1/L2 and L1/L2/L5 versions of their antennas.

POWER REQUIREMENTS: 
In standard configuration, this system is powered by a 12V lead-acid battery. 
  
While this system is optimized for 12V batteries, input voltage can range from 5.2V to 20V with the following considerations/customizations:  
  - The DC-DC converter minimum input is 5.2V and maximum input is 50V, although additional power filtering at high voltages is required. 
  - The Battery Measurement circuit features a voltage divider circuit that must scale max voltage to 3.3V for the ADC pin. Standard dividers for a 12V battery use 68kOhm and 10kOhm resistors. USER may wish to adjust gain/offset of ADC battery measurement conversion in software if using non-standard (5.2-20V) power configuration. 
  - The reverse polarity protection MOSFET has a limit of 20V. **Do not exceed 20V** without either removing this part or finding an appropriate substitute component. 

## License & Credits
This project is open source! OGRE software is released under the [MIT License](http://opensource.org/licenses/MIT).

Portions of this code for this project are derived from [Sparkfun GNSS Library](https://github.com/sparkfun/SparkFun_u-blox_GNSS_Arduino_Library) [Sparkfun Arduino Core](https://github.com/sparkfun/Arduino_Apollo3/releases/tag/v1.2.0), [Sparkfun OpenLog GNSS](https://github.com/sparkfun/OpenLog_Artemis_GNSS_Logger), and
[Cryologger Glacier Velocity Tracker v2.0.3](https://github.com/adamgarbo/Cryologger_Glacier_Velocity_Tracker).

Modifications Copyright (c) 2020 SparkFun
Modifications Copyright (c) 2020 Adam Garbo

See [License](LICENSE.md) for more details.
