
<p align="center">
<img src="https://github.com/glaciology/OGRE/assets/37055625/38e84161-e7fa-485c-b498-54799fec6b80" width="250"/>
 <img src="https://user-images.githubusercontent.com/37055625/181100467-84fbc824-24a9-4d06-a20b-8dd893361870.PNG" width="250"/>
 </p>


# Open GNSS Research Equipment :: A low-power, low-cost GNSS data logger for monitoring the cryosphere.
<p align="center">
<img src="https://github.com/user-attachments/assets/352f2a3b-dfde-4013-85c5-045853ee4342" width="500"/>
</p>


## Overview
Originally designed for easily logging multi-GNSS data in remote regions of the Arctic, this instrument incorporates low-power, low-cost components onto a single circuit board and features a u-blox ZED-F9P, ZED-F9T or ZED-X20P GNSS module and an Ambiq Apollo3 MCU. By stripping away many of the features of commercial options, we streamline this instrument specifically for "set it and forget it" rapid deployments, acquiring high quality GNSS data.
- Logs GPS, GLONASS, BEIDOU, GALILEO, QZSS & Satellite Nav Messages to a microSD, which can be easily converted to standard RINEX format (from .ubx) for positioning with PPK or PPP (RTK is possible but DIY).
- Hardware v1 (SparkFun Artemis module, 1 LED) supports the ZED-F9P and ZED-F9T. Hardware v2 (Apollo3 MCU, 2 LEDs) supports the ZED-F9P and ZED-X20P.
- Signals: ZED-F9P logs L1/L2. ZED-F9T logs L1/L2 or L1/L5, as configured on the module. ZED-X20P logs L1/L2/L5; L5 is enabled automatically on the X20P only.
- Easily configurable via microSD card. Many configuration options to optimize data and battery needs, such as a polar-specific mode of daily logging during summertime and interval logging during the winter, or a twice-daily mode for capturing diurnal glacial signals.
- Low cost, totaling ~$175, not including external components. PCB inside enclosure measures 7x6.5x2.5cm.
- Current consumption with a 12V supply is 45-65mA (.5-.8W) awake, and 0.07mA (.8mW) asleep. We have used a single 12V 40Amp hr battery with a 10W solar panel to log continuously for 3 hours a day every day, including throughout the long polar night in Greenland.
- Programmed in Arduino environment, for easy customization.

A detailed user guide is in this repository: [OGRE User Guide](Documentation/OGRE_User_Guide.md) (also depricated [here](https://docs.google.com/document/d/1q8-jAFY2cO8MIOHwoCQhJwPjmURbkkGEsrDBrOs5gMM/edit?usp=sharing)).

<p align="center">
<img src="https://user-images.githubusercontent.com/37055625/207705142-4ba32b05-6b62-4d18-bcf7-4f9eb635589c.jpeg" width="500"/>
</p>

## Project Organization
[Software: Contains Arduino Code and Test Scripts](Software/)
- [OGRE: Software and software config files for upload to OGRE instrument](Software/OGRE) <br>

[Hardware: Hardware & Manufacturing Files](Hardware/)
- Here you will find the component lists for OGREv1 and OGREv2. We recommend all future acquisitions use OGREv2. However, use v1 if you or your manufacturer cannot flash the device using a SWD/JTAG interface. See [Software Upload](#software-upload) for details on flashing the correct firmware onto your v1 or v2 device.

[Documentation: User Guide and Firmware Upload Guide](Documentation/)

Find the full docs here: [here](Documentation/OGRE_User_Guide.md)!

## Getting Started

### Log Modes
The OGRE has the following modes of operation (not all modes are available on older firmware; see [Firmware Versions & CONFIG.TXT Compatibility](#firmware-versions--configtxt-compatibility)):
  - (1) Daily Fixed Mode: Log GNSS data every day, starting at `LOG_START_HOUR_UTC` and ending at `LOG_END_HOUR_UTC`.
    - Sessions start and end on the hour (UTC). Windows that cross midnight are supported (e.g., start 22, end 4).
    - Start and end hours must differ.
  - (2) Continuous Mode: Log GNSS data continuously; a new file is generated at each midnight UTC.
    - The RTC is re-synced at each midnight UTC, leaving a gap of a few seconds between files.
    - The system does not sleep in this mode (except for low battery).
  - (3) Monthly Mode: Log GNSS data for 24 hours on a USER-specified day (`LOG_START_DAY`, 1-28) each month.
    - Sessions start at 00:00 UTC on `LOG_START_DAY` and last 24 hours from wake-up, ending on the following UTC date.
  - (4) Interval Mode: Log GNSS data for 24 hours, then sleep for `LOG_EPOCH_SLEEP` seconds, and repeat (e.g., log every 3 days for 24 hours).
    - The sleep interval is counted from the end of each session: one cycle = 24 hours + `LOG_EPOCH_SLEEP` + RTC sync time.
    - Session start times shift later each cycle by the RTC sync time.
    - Sleep intervals longer than 1 year are not supported.
  - (5) Date Mode: Log GNSS data for 24 hours starting at USER-specified dates/times read from EPOCH.TXT. Defaults to mode 4 after the last date.
    - Dates must be listed in ascending order.
    - Each date starts a 24-hour session; a date that falls within a running session is skipped.
    - Dates more than 1 year in the future are not supported.
    - If EPOCH.TXT is missing or invalid, mode 4 behavior is used from the start.
  - (6) Seasonal Mode: During summer, log continuously (new file each midnight UTC). During winter, log for 24 hours, then sleep for `WINTER_INTERVAL` seconds.
    - Summer runs from `SUMMER_START_MONTH`/`SUMMER_START_DAY` through `SUMMER_END_MONTH`/`SUMMER_END_DAY`, inclusive (default April 21 - September 21).
    - Summers that wrap the new year (start month > end month, e.g., southern hemisphere) are supported.
    - Summer start and end days are limited to 1-28.
    - The winter sleep interval is counted from the end of each session. If summer begins during a winter sleep, the system wakes at the start of summer.
  - (7) Reserved for development. Not for deployment.
  - (8) Twice-Daily Mode: Log GNSS data twice daily, from `LOG_START_HOUR_UTC` to `LOG_END_HOUR_UTC` and from `LOG_START_HOUR_TWO_UTC` to `LOG_END_HOUR_TWO_UTC`.
    - Both sessions must fall within one UTC day, in chronological order, without touching (end of session 1 < start of session 2). Sessions crossing midnight are not supported.
    - Available in firmware V3.2.0 and later.
  - (99) Test Mode: Used for development. Log GNSS data for 50 seconds, sleep for 50 seconds, and repeat.

ALL MODES:
- Each session begins with an RTC sync, which requires a 3D fix with confirmed date and time. Logging begins once the sync completes (up to 3 minutes after wake-up).
- All times are UTC.
- Modes 1, 3 and 8: if the system powers on outside a log window, it writes a short .ubx file and a debug line, then sleeps until the next window. This confirms the system initialized.

### OUTPUTs
With all modes, GNSS data (phase, doppler, SNR, nav message etc.) are logged to a uSD card in .ubx (UBLOX) proprietary format. Under open sky conditions, we found that an epoch of data from a ZED-F9P (1s) is ~2000-3000 bytes. If logging at 15 seconds for a year, this equates to 6GB of data.

GNSS DATA FILES:
- Named `SSSS_YYYYMMDD_HHMM.ubx`, where `SSSS` is `STATION_NAME` and `YYYYMMDD_HHMM` is the UTC session start time. All files are written to the root of the uSD card.
- Contain UBX-RXM-RAWX at `MEASURE_RATE`, plus UBX-RXM-SFRBX if `ENABLE_NAV_SFRBX=1`.
- Two sessions starting in the same minute are written to the same file.
- Data are saved to the card every 5 seconds; a power loss can lose up to the last 5 seconds of data.

DEBUG FILE:
- Named `debug_<chipID>.csv`, where `<chipID>` is the MCU's unique ID in hex. One file per instrument, independent of `STATION_NAME`.
- A header line is appended at every power-on or reset. It contains the software version, the column names, and the reset cause (e.g., `POIRSTAT` power-on, `WDRSTAT` watchdog reset).
- A data line is appended at the end of each log session and at each error.
- Columns: date/time, debugCounter, onlineGNSS, onlineSD, rtcSync, rtcDrift (seconds corrected at the last RTC sync), bytesWritten, maxBufferBytes, wdtCounterMax, writeFailCounter, syncFailCounter, closeFailCounter, LogMode, errorCode, lowBattery, Temperature (MCU, °C), Battery (V, only if `MEASURE_BATTERY=1`).
- Date/time on lines written before the first RTC sync after power-on is not valid.

| errorCode | Meaning |
|---|---|
| `success` | Log session completed |
| `GNSS_<module>_<fw>_<version>_L2=_L5=_setL5=` | Once per power-on: detected GNSS module, firmware, the L2/L5 state at power-on, and whether L5 was enabled by the OGRE |
| `GNSS_ID_FAIL_...` | GNSS module could not be identified; L5 not enabled |
| `CFG_FILE_ERROR` | CONFIG.TXT has missing, invalid or unrecognized settings; defaults used for those settings |
| `CFG_FAIL_n` | `n` constellation settings were rejected by the GNSS module |
| `I2C` | GNSS module not detected |
| `RTC_SYNC` | RTC sync failed within 3 minutes |
| `GNSS_FILE_CREATE` | GNSS data file could not be created |

SERIAL OUTPUT (`OUTPUT_UART_RAWX=1`):
- Raw UBX data of all logged messages (RAWX, plus SFRBX if enabled) are written to the serial port at 115200 baud, in addition to the uSD card.
- Debug text is written to the same serial port.
- Disabled automatically if `MEASURE_RATE` < 5.

### INPUTs
USERS specify settings in the CONFIG.TXT file on the uSD card. If no CONFIG.TXT exists on the SD card, the default settings are used and a CONFIG.TXT with the default settings is written to the SD card. **Recommended for all firmware versions:** power on the OGRE with an empty uSD card to let it generate its own CONFIG.TXT, then edit the values in that file. This guarantees the file matches the installed firmware. USER may also upload an EPOCH.TXT file, which allows the user to specify up to 20 log dates (unix epoch format) for logging in Mode 5. CONFIG.TXT and EPOCH.TXT are read only at power-on or reset.

The CONFIG.TXT file written by firmware V3.2.0 (with default values):

```
LOG_MODE(1: daily hr, 2: cont, 3: mon, 4: 24 roll, 5: date, 6: season, 8: 2x daily)=6
LOG_START_HOUR_UTC(mode 1&8 only)=12
LOG_END_HOUR_UTC(mode 1&8 only)=16
LOG_START_DAY(mode 3 only, 1-28)=8
LOG_EPOCH_SLEEP(modes 4/5 only, seconds)=518400
LED_INDICATORS(0-false, 1-true)=1
MEASURE_BATTERY(0-false, 1-true)=0
ENABLE_GPS(0-false, 1-true)=1
ENABLE_GLO(0-false, 1-true)=1
ENABLE_GAL(0-false, 1-true)=1
ENABLE_BDS(0-false, 1-true)=1
ENABLE_QZSS(0-false, 1-true)=0
ENABLE_NAV_SFRBX(0-false, 1-true)=0
STATION_NAME(0000, char)=0000
MEASURE_RATE(integer seconds)=5
BAT_SHUTDOWN_V(00.0, volts)=10.90
WINTER_INTERVAL(seconds, mode 6 only)=777600
SUMMER_START_MONTH(mode 6 only)=4
SUMMER_END_MONTH(mode 6 only)=9
SUMMER_START_DAY(mode 6 only)=21
SUMMER_END_DAY(mode 6 only)=21
OUTPUT_UART_RAWX=0
LOG_START_HOUR_TWO_UTC(mode 8 only)=16
LOG_END_HOUR_TWO_UTC(mode 8 only)=19
V3.2.0 end;
```

| Setting | Valid values | Default | Used by |
|---|---|---|---|
| `LOG_MODE` | 1, 2, 3, 4, 5, 6, 7, 8, 99 | 6 | All |
| `LOG_START_HOUR_UTC` | 0-23 | 12 | Modes 1, 8 |
| `LOG_END_HOUR_UTC` | 0-23 | 16 | Modes 1, 8 |
| `LOG_START_DAY` | 1-28 | 8 | Mode 3 |
| `LOG_EPOCH_SLEEP` | Seconds, ≥1; ≤31536000 (1 year) supported | 518400 (6 days) | Modes 4, 5 |
| `LED_INDICATORS` | 0, 1 | 1 | All |
| `MEASURE_BATTERY` | 0, 1 | 0 | All |
| `ENABLE_GPS` | 0, 1 | 1 | All |
| `ENABLE_GLO` | 0, 1 | 1 | All |
| `ENABLE_GAL` | 0, 1 | 1 | All |
| `ENABLE_BDS` | 0, 1 | 1 | All |
| `ENABLE_QZSS` | 0, 1 | 0 | All |
| `ENABLE_NAV_SFRBX` | 0, 1 | 0 | All |
| `STATION_NAME` | 4 characters, letters and digits only | 0000 | All |
| `MEASURE_RATE` | 1-60 (integer seconds) | 5 | All |
| `BAT_SHUTDOWN_V` | 5.0-16.0 (volts) | 10.9 | `MEASURE_BATTERY=1` |
| `WINTER_INTERVAL` | Seconds, ≥1; ≤31536000 (1 year) supported | 777600 (9 days) | Mode 6 |
| `SUMMER_START_MONTH` | 1-12 | 4 | Mode 6 |
| `SUMMER_END_MONTH` | 1-12 | 9 | Mode 6 |
| `SUMMER_START_DAY` | 1-28 | 21 | Mode 6 |
| `SUMMER_END_DAY` | 1-28 | 21 | Mode 6 |
| `OUTPUT_UART_RAWX` | 0, 1 | 0 | All |
| `LOG_START_HOUR_TWO_UTC` | 0-23 | 16 | Mode 8 |
| `LOG_END_HOUR_TWO_UTC` | 0-23 | 19 | Mode 8 |

- LED_INDICATORS, if false, disables the sleep and logging LEDs. Initialization and error patterns remain.
- MEASURE_BATTERY, if true, battery voltage is measured at the start of each log session, and the instrument sleeps when voltage is below `BAT_SHUTDOWN_V` (default 10.9V). The system resumes when voltage reaches `BAT_SHUTDOWN_V` + 0.5V (11.4V at default). Voltage is not monitored during a session.
- STATION_NAME is appended to the file name of each GNSS file. Names longer than 4 characters are truncated. Characters such as `/ \ : * ? " < > |` and spaces prevent file creation.
- MEASURE_RATE is the interval between epochs logged to the SD card: 1 = 1 epoch per second, 15 = 1 epoch per 15 seconds.
- ENABLE_GLO on the ZED-X20P is applied if supported by the module firmware; a rejection is recorded in the debug file as `CFG_FAIL_n`.

CONFIG.TXT RULES (firmware V3.2.0):
- Each line has the form `SETTING(description)=value`. The text in parentheses is ignored and may be omitted.
- Settings are matched by name; line order does not matter.
- Setting names are case-sensitive. No spaces before `(` or `=`.
- Lines are limited to 99 characters.
- Lines without `=` (e.g., the version line) and blank lines are ignored.
- A missing or invalid setting keeps its default value. Missing, invalid and unrecognized settings are recorded in the debug file as `CFG_FILE_ERROR`. There is no LED indication.
- If a setting appears more than once, the last value is used.
- Values are read as integers (except `BAT_SHUTDOWN_V` and `STATION_NAME`). Reading stops at the first non-digit: `86,400` is read as 86. Non-numeric text is read as 0, which is a valid value (disabled) for 0/1 settings.
- Save as plain text (ANSI or UTF-8 without BOM). A UTF-8 BOM causes the first line (`LOG_MODE`) to be ignored. On macOS TextEdit, use Format > Make Plain Text.
- The file must be named `CONFIG.TXT` (not case-sensitive). On Windows, check that the file is not saved as `CONFIG.txt.txt`.
- Windows and Mac line endings are supported.

EPOCH.TXT (mode 5):
- Exactly 20 lines, each `label=unix_epoch` (e.g., `1=1656348647`). Unused lines are set to `0`.
- A file with any other number of lines (including a trailing blank line) is ignored: the instrument blinks 5 times and uses mode 4 behavior.
- Read only when `LOG_MODE=5`.

### Firmware Versions & CONFIG.TXT Compatibility
CONFIG.TXT files are not interchangeable between all firmware versions. For V3.0.1, V3.2.0 and later, let the OGRE generate its own CONFIG.TXT: power on the instrument without a CONFIG.TXT on the uSD card, then edit the values in the generated file.

| | V3.0.1 | V3.2.0 |
|---|---|---|
| Hardware supported | v1 only | v1 and v2 |
| GNSS modules | ZED-F9P, ZED-F9T | ZED-F9P, ZED-F9T, ZED-X20P |
| CONFIG.TXT lines | Exactly 22 (21 settings + version line) | Any; matched by name |
| Settings read by | Line position | Setting name |
| Log modes | 1-7, 99 | 1-8, 99 |
| `OUTPUT_UART_RAWX`, `LOG_START_HOUR_TWO_UTC`, `LOG_END_HOUR_TWO_UTC` | Not available | Available |
| Range checks | `LOG_MODE`, 0/1 settings, `MEASURE_RATE` ≥1 only | All settings |
| L5 | Set at compile time | Automatic (ZED-X20P only) |
| Default `LOG_MODE` | 2 | 2 (subject to change) |
| u-blox library | SparkFun u-blox GNSS v2.2.8 | SparkFun u-blox GNSS v3 (≥3.1.15) |

V3.0.1 LIMITATIONS:
- A CONFIG.TXT with any number of lines other than 22 (including a trailing blank line) is rejected entirely: all settings use defaults, the instrument blinks 6 times, and `CFG_FILE_VERSION` is recorded in the debug file.
- Settings are read by position: lines must be in the V3.0.1 order. The setting names are not checked, except `STATION_NAME(0000, char)` and `BAT_SHUTDOWN_V(00.0, volts)`, which must match exactly (including the text in parentheses) or the default is used.
- No range checks on `LOG_START_HOUR_UTC`, `LOG_END_HOUR_UTC`, `LOG_START_DAY`, `LOG_EPOCH_SLEEP`, `WINTER_INTERVAL`, or the `SUMMER_` settings. Out-of-range values are used as entered.
- Mode 1: at power-on, logs from power-on until `LOG_END_HOUR_UTC`, regardless of the start hour.
- Mode 3: at power-on, logs 24 hours from power-on, regardless of the day.
- A rejected constellation setting halts the instrument (3 blink pattern) until watchdog reset.

COMPATIBILITY:
- V3.0.1 CONFIG.TXT on V3.2.0: accepted. `OUTPUT_UART_RAWX`, `LOG_START_HOUR_TWO_UTC` and `LOG_END_HOUR_TWO_UTC` use defaults, and `CFG_FILE_ERROR` is recorded in the debug file. Values outside the V3.2.0 ranges use defaults.
- V3.2.0 CONFIG.TXT on V3.0.1: rejected (25 lines). All settings use V3.0.1 defaults.

### BASIC OPERATION
Insert the uSD card (with or without CONFIG & EPOCH files), then connect battery to +/- sides of the screw terminal. The system will attempt to initialize and the following LED indicators will flash:
  - 1 Hz Blinks: System acquiring GPS time and attempting to sync real time clock (RTC).
  - 10 rapid Blinks: System Configuration Complete!

The following LED patterns indicate success or failure. OGREv1 has 1 LED (all patterns on that LED). OGREv2 has 2 LEDs (red and yellow).

| Pattern | Meaning | Color (v2) | Triggered From |
|---|---|---|---|
| 2 blink pattern, repeating | microSD card failed to initialize (twice) | 🔴 Red (`LED`) | `configureSD()` |
| 3 blink pattern, repeating | GNSS module not detected on I2C, or GNSS data file could not be created | 🔴 Red (`LED`) | `configureGNSS()` / `logGNSS()` |
| 5 blink pattern, repeating | RTC failed to sync with GNSS within 3 minutes | 🔴 Red (`LED`) | `syncRtc()` |
| 5 rapid blinks, once | EPOCH.TXT missing or wrong number of lines (mode 5 only) | 🔴 Red (`LED`) | `getDates()` |
| 6 rapid blinks, once | `CONFIG.TXT` missing — using defaults; default `CONFIG.TXT` written | 🔴 Red (`LED`) | `getConfig()` |
| 10 blinks | RTC synced + system configuration complete (power-on/reset only) | 🟡 Yellow (`LED2`) | `setup()` |
| 1 blink, roughly every 12 seconds | Device sleeping between sessions (`LED_INDICATORS=1`) | 🟡 Yellow (`LED2`) | `loop()` |
| 1 Hz blink | Acquiring GNSS fix during RTC sync | 🟡 Yellow (`LED2`) | `syncRtc()` |
| Rapid, irregular blinks while active | Logging; one blink per 512-byte write to SD (`LED_INDICATORS=1`) | 🟡 Yellow (`LED2`) | `logGNSS()` |
| No blinks at all | Deep sleep — battery below shutdown threshold, or `LED_INDICATORS=0` | *(none — both LEDs off)* | `checkBattery()` / `goToSleep()` |

- Repeating patterns continue until the watchdog resets the system (~1.5 minutes), after which initialization restarts.
- After an RTC sync failure, the system restarts and retries until a fix is obtained. The system does not sleep and draws full power during retries. The battery shutdown threshold is not applied during retries.

### Limitations
- RAWX epochs with more than 120 measurements are not logged (SparkFun u-blox GNSS v3.1.15 limit).
- Battery voltage is checked at the start of each session only.
- The uSD card's free space is not checked. When the card is full, writes fail and are counted in `writeFailCounter`.
- Sleep intervals and EPOCH.TXT dates more than 1 year ahead are not supported.
- CONFIG.TXT and EPOCH.TXT changes take effect only after power-on or reset.

## Software Upload
Only do this if you want to update the firmware on the OGRE, or if the OGRE has not yet had the firmware installed. Pre-compiled binary files are available with each release (see [releases](https://github.com/glaciology/OGRE/releases/)).

Binaries differ by hardware version and by start address. Use the binary that matches both your board and its bootloader:

| Board | Bootloader on MCU | Binary start address | Upload method |
|---|---|---|---|
| OGREv1 (Artemis module) | SparkFun Variable Loader (SVL), installed on Artemis modules | 0x10000 | USB-to-Serial, `svl.py` |
| OGREv2 (Apollo3), newly manufactured | None (Ambiq secure bootloader only) | 0xC000 | SWD/JTAG |
| OGREv2 (Apollo3) | SVL installed | 0x10000 | USB-to-Serial, `svl.py` |

- A 0x10000 binary does not run on an MCU without the SVL.
- Writing a 0xC000 binary over SWD/JTAG overwrites the SVL. Subsequent uploads require SWD/JTAG.
- A v1 binary on a v2 board (or a v2 binary on a v1 board) uses the wrong pin assignments; the instrument does not operate.

Step-by-step instructions for both methods (USB-to-Serial with the SVL, or SWD with a SEGGER J-Link), including how to install the SVL on an OGREv2, are in the [OGRE Firmware Upload Guide](Documentation/OGRE_Firmware_Upload_Guide.md). A newly manufactured OGREv2 can only be programmed over SWD/JTAG unless you have the manufacturer pre-upload the SVL bootloader or the OGRE firmware.

### Compiling
Compile the source code with the Arduino IDE, using the versions defined in the header of OGRE.ino:
- SparkFun Apollo3 Arduino Core v1.2.3
- SdFat library v2.1.0
- SparkFun u-blox GNSS v3 library, v3.1.15 or later (both hardware versions)

Set the `HARDWARE_VERSION` macro in OGRE.ino before compiling: `1` = OGREv1, `2` = OGREv2. `HARDWARE_VERSION` selects the pin assignments and LEDs. GNSS module-specific settings (L5) are detected at run time. The linker start address (0x10000 or 0xC000) must match the target bootloader.

## Hardware Notes
<p align="center">
<img src="https://github.com/user-attachments/assets/226c09d0-3a6c-419c-b80c-52f5dac20911" width="250"/>
</p>

| | OGREv1 | OGREv2 |
|---|---|---|
| MCU | SparkFun Artemis module (Apollo3) | Ambiq Apollo3 |
| LEDs | 1 | 2 (red, yellow) |
| GNSS module | ZED-F9P, ZED-F9T | ZED-F9P, ZED-X20P |
| Bootloader | SVL (installed on Artemis modules) | None by default (SWD/JTAG upload) |
| Firmware | All versions | V3.2.0 and later |

MATERIALS
Cost of PCB and all components totals ~$175. Detailed list of components found in the Hardware folder. <br>

ASSEMBLY
Assembly services are available through PCBWay, and the OGRE can be ordered directly from [here](https://www.pcbway.com/project/shareproject/OGRE_Open_GNSS_Research_Equipment_Receiver_33a809f3.html) for V1 or from [here](https://www.pcbway.com/project/shareproject/OGRE_Open_GNSS_Research_Equipment_v2_0_82247d12.html) for V2. Note that this assembly service often does not include the [Pololu](https://www.pololu.com/product/3792) component, which must be soldered (4 through-hole) by the user (requests may be made to PCBWay to see if they can source this component, or the user can ship to PCBWay). The user must also program the device following the instructions from the [Software Upload](#software-upload) section. For OGREv2, we highly recommend the customer asks PCBWay to upload the .bin firmware to the board using their JTAG/SWD interface. Instructions for PCBWay upload can be found in the [OGRE Firmware Upload Guide](Documentation/OGRE_Firmware_Upload_Guide.md) (Option B).

If you choose to use a u-blox antenna, you will need a ground plane, which you can make out of any metal disc, or you can order pre-drilled discs from PCBWay too, [here](https://www.pcbway.com/project/shareproject/Ground_Plane_for_ublox_L1_L2_L5_GNSS_antenna_c3519487.html), compatible for both L1/L2 and L1/L2/L5 versions of their antennas.

POWER REQUIREMENTS:
In standard configuration, this system is powered by a 12V lead-acid battery.

While this system is optimized for 12V batteries, input voltage can range from 5.2V to 20V with the following considerations/customizations:
  - The DC-DC converter minimum input is 5.2V and maximum input is 50V, although additional power filtering at high voltages is required.
  - The Battery Measurement circuit features a voltage divider circuit that must scale max voltage to the ADC input range (0-2.0V). Standard dividers for a 12V battery use 68kOhm and 10kOhm resistors; with the default gain/offset, readings saturate at ~17.4V. USER may wish to adjust gain/offset of ADC battery measurement conversion in software if using non-standard (5.2-20V) power configuration.
  - The reverse polarity protection MOSFET has a limit of 20V. **Do not exceed 20V** without either removing this part or finding an appropriate substitute component.

## License & Credits
This project is open source! OGRE software is released under the [MIT License](http://opensource.org/licenses/MIT).

Portions of this code for this project are derived from [Sparkfun GNSS Library](https://github.com/sparkfun/SparkFun_u-blox_GNSS_Arduino_Library) [Sparkfun Arduino Core](https://github.com/sparkfun/Arduino_Apollo3/releases/tag/v1.2.0), [Sparkfun OpenLog GNSS](https://github.com/sparkfun/OpenLog_Artemis_GNSS_Logger), and
[Cryologger Glacier Velocity Tracker v2.0.3](https://github.com/adamgarbo/Cryologger_Glacier_Velocity_Tracker).

Modifications Copyright (c) 2020 SparkFun
Modifications Copyright (c) 2020 Adam Garbo

See [License](LICENSE.md) for more details.
