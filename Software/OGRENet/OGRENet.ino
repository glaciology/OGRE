/*
   OGRENet: On-ice GNSS Research Experimental Network for Greenland
   Derek Pickell 01/20/24
   V2.0.1

   Hardware:
   - OGRENet PCB w/ ZED-F9P/T (Note: using -F9T, adjust L5 settings).

   Dependencies:
   - SparkFun_u-blox_GNSS_Arduino_Library v2.2.8
   - Apollo3 Arduino Core v1.2.3
   - SdFat Library v2.1.0

   INSTRUCTIONS:
   - See github.com/glaciology/OGRENet/ for detailed instructions.
   - Optional settings files on SD card: CONFIG.txt, EPOCHS.txt
   - LED indicators:
        *2 Blink Pattern: uSD failed - waiting for RESET
        *3 Blink Pattern: Ublox I2C or config failed - waiting for RESET
        *5 Blink Pattern: RTC sync fail - waiting for RESET
        *6 Rapid Blinks: CONFIG file failed to read; using defaults
        *10 Blinks: RTC synced and System Configuration COMPLETE (After Initial Power On or Reset Only)
        *1 Blink every 12 seconds: Sleeping 
        *Random Rapid Blinks: System logging data.
        *No Blinks: System deep sleep due to low battery or battery dead.

   Attributions: 
   - Files, structure and ideas adapted/modified from Sparkfun 
   - and A. Garbo GVMS [Copyright (c) 2020 SparkFun, Copyright (c) 2020 Adam Garbo]
   - are dilineated in these pages, and further detailed in the documentation. 
   - This project is open source; see Readme/Licensing.
*/

#define HARDWARE_VERSION 1  // 1 = CUSTOM DARTMOUTH HARDWARE v3/22 - present
#define SOFTWARE_VERSION "2-0-1" 

///////// LIBRARIES & OBJECT INSTANTIATIONS //////////
#include <Wire.h>                             // Apollo3 Arduino Core v1.2.3
#include <SPI.h>                              // ""
#include <WDT.h>                              // ""
#include <RTC.h>                              // ""
#include <SdFat.h>                            // https://github.com/greiman/SdFat v2.1.0
#include <SparkFun_u-blox_GNSS_Arduino_Library.h> 
SFE_UBLOX_GNSS gnss;                          // Library v2.2.8: http://librarymanager/All#SparkFun_u-blox_GNSS
SdFs sd;                                      // SdFs = supports FAT16, FAT32 and exFAT (4GB+), corresponding to FsFile class
APM3_RTC rtc;                                 //
APM3_WDT wdt;                                 // 
FsFile myFile;                                // RAW UBX LOG FILE
FsFile debugFile;                             // DEBUG LOG FILE
FsFile configFile;                            // USER INPUT CONFIG FILE
FsFile dateFile;                              // USER INPUT EPOCHS FILE

///////// HARDWARE-SPECIFIC PINOUTS & OBJECTS ////////
const byte BAT                    = 35;       //
const byte SHIELD                 = 19;       // Use with Telemetry Shield
const byte PER_POWER              = 18;       // Drive to turn off uSD
const byte ZED_POWER              = 34;       // Drive to turn off ZED
const byte LED                    = 33;       //
const byte PIN_SD_CS              = 41;       //
const byte BAT_CNTRL              = 22;       // Drive high to turn on Bat measure

TwoWire myWire(2);                            // USE I2C bus 2, SDA/SCL 25/27
SPIClass mySpi(3);                            // Use SPI 3 - pins 38, 41, 42, 43
#define SD_CONFIG SdSpiConfig(PIN_SD_CS, DEDICATED_SPI, SD_SCK_MHZ(24), &mySpi)

//////////////////////////////////////////////////////
//--------- USER DEFAULT CONFIGURATION HERE ------------
// LOG MODE: 
byte logMode                = 6;              // {1, 2, 3, 4, 5, 6, 99}

// LOG MODE 1: DAILY, DURING DEFINED HOURS
byte logStartHr             = 12;             // UTC Hour 
byte logEndHr               = 14;             // UTC Hour

// LOG MODE 3: ONCE/MONTH FOR 24 HOURS
byte logStartDay            = 8;              // Day of month between 1 and 28

// LOG MODE 4/5: SLEEP FOR SPECIFIED DURATION
uint32_t epochSleep         = 2628000;        // Sleep duration (Seconds) (i.e., 2628000 ~ 1 month)

// LOG MODE 6: SUMMER/WINTER: LOG 24 HOURS, SLEEP FOR:
uint32_t winterInterval     = 777600;         // Sleep duration during winter, i.e., 9 days
byte startMonth             = 4;              // Start month, inclusive (May)
byte endMonth               = 9;              // End month, inclusive (August)
 
// LOG MODE 99: TEST: ALTERNATE SLEEP/LOG FOR X SECONDS
uint32_t secondsSleep       = 50;             // Sleep interval (Seconds)
uint32_t secondsLog         = 50;             // Logging interval (Seconds)

// UBLOX MESSAGE CONFIGURATION: 
int logGPS                  = 1;              // FOR EACH CONSTELLATION 1 = ENABLE, 0 = DISABLE
int logGLO                  = 1;              //
int logGAL                  = 1;              //
int logBDS                  = 1;              //
int logQZSS                 = 0;              //
int logSBAS                 = 0;              // Not on SD CONFIG File 
int logNav                  = 0;              //
int logL5                   = 0;              // WARNING: only set if using L5-capable ZED.

// ADDITIONAL CONFIGURATION
bool ledBlink               = true;           // If FALSE, all LED indicators during log/sleep disabled
bool measureBattery         = false;          // If TRUE, uses battery circuit to measure V during debug logs
int  stationName            = 0000;           // Station name, 4 digits
int  measurementRate        = 5;              // Produce a measurement every X seconds

// BATTERY PARAMETERS
float gain                   = 17.2;          // Gain/offset for 68k/10k voltage divider battery voltage measure
float offset                 = 0.23;          // ADC range 0-2.0V
float shutdownThreshold      = 10.9;          // Shutdown if battery voltage dips below this (10.8V for DEKA 12V GEL)
                                              // SYSTEM WILL SLEEP IF DIPS BELOW HERE, WAKES after shutdownThreshold + 0.5V reached
//----------------------------------------------------
//////////////////////////////////////////////////////

///////// GLOBAL VARIABLES ///////////////////////////
const int     sdWriteSize         = 512;      // Write data to SD in blocks of 512 bytes
const int     fileBufferSize      = 16384;    // Allocate 16KB RAM for UBX message storage
unsigned int  rtcSyncDay          = 0;        // Ensures only 1 file/day for LM 2, 6
bool          summerInterval      = false;    // LM 6 only: when true, it's summer so it logs continuously
volatile bool wdtFlag             = false;    // ISR WatchDog
volatile bool alarmFlag           = true;     // RTC alarm true when interrupt (initialized as true for first loop)
volatile bool initSetup           = true;     // False once GNSS messages configured-will not configure again
unsigned long prevMillis          = 0;        // Global time keeper, not affected by Millis rollover
unsigned long dates[21]           = {};       // Array with Unix Epochs of log dates !!! MAX 20 !!!
int           settings[20]        = {};       // Array that holds user settings on SD
char          line[100];                      // Temporary array for parsing user settings
char          logFileNameDate[30] = "";       // Log file name

// DEBUGGING
unsigned long maxBufferBytes      = 0;        // How full the file buffer has been
unsigned long bytesWritten        = 0;        // used for printing to Serial Monitor bytes written to SD
unsigned int  syncFailCounter     = 0;        // Counts RTC sync failures
unsigned int  writeFailCounter    = 0;        // Counts uSD write failures
unsigned int  closeFailCounter    = 0;        // Counts uSD close failures
unsigned int  lowBatteryCounter   = 0;        // Counts # times system sleeps due to low battery
unsigned int  debugCounter        = 0;        // Counts Debug messages
volatile int  wdtCounter          = 0;        // Counts WDT triggers
volatile int  wdtCounterMax       = 0;        // Tracks Max times WDT interrupts
long          rtcDrift            = 0;        // Tracks drift of RTC

struct struct_online {
  bool uSD      = false;
  bool gnss     = false;
  bool rtcSync  = false;                      
} online;
//////////////////////////////////////////////////////

///////// DEBUGGING MACROS ///////////////////////////
#define DEBUG                     true        // Output messages to Serial monitor
#define DEBUG_GNSS                false       // Output GNSS debug messages to Serial monitor

#if DEBUG
#define DEBUG_PRINTLN(x)          Serial.println(x)
#define DEBUG_PRINT(x)            Serial.print(x)

#else
#define DEBUG_PRINTLN(x)
#define DEBUG_PRINT(x)
#endif
//////////////////////////////////////////////////////
//////////////////////////////////////////////////////

void setup() {
  #if DEBUG
    Serial.begin(115200);
    delay(1000);
    Serial.println("***WELCOME TO GNSS LOGGER v2.0.1 (1/24)***");
  #endif

  //// CONFIGURE INITIAL SETTINGS  ////
  pinMode(LED, OUTPUT);              //
  pinMode(LED, HIGH);                //
  configureWdt();                    // 12s interrupt, 24s reset period
  checkBattery();                    // IF battery LOW, send back to sleep until recharged
  initializeBuses();                 // Initializes I2C & SPI and turns on ZED (I2C), uSD (SPI)
  configureSD();                     // BLINK 2x pattern - FAILED SETUP
  getConfig();                       // Read LOG settings from Config.txt on uSD; 5x - FAILED
  createDebugFile();                 // Creates debug file headers
  getDates();                        // Read Dates (in unix epoch format) for log mode 5
  configureGNSS();                   // BLINK 3x pattern - FAILED SETUP
  syncRtc();                         // 1Hz BLINK-AQUIRING; 5x - FAIL (3 min MAX)

//------------LOG MODE-SPECIFC SETTINGS----------------
  if (logMode == 1 || logMode == 3) {                    
       configureSleepAlarm();        // Get ready to sleep for these modes
       deinitializeBuses();
  }
//----------------------------------------------------  

  blinkLed(10, 100);                 // BLINK 10x - SETUP COMPLETE
  DEBUG_PRINTLN("Info: SETUP COMPLETE");
}


void loop() {
  
    if (alarmFlag) {                 // SLEEPS until alarmFlag = True
      checkBattery();                //
      petDog();                      //
      if (online.gnss == false || online.uSD == false) {
        initializeBuses();           // Reconfigure GNSS/SD if necessary
        configureSD();               //
        configureGNSS();             //
      }                              //

      if (!online.rtcSync) {        // sync clock if hasn't happened
        syncRtc();                   // flag is reset in deinitializeBuses()
      }
      
      configureLogAlarm();           // 
      logGNSS();                     //
      
      DEBUG_PRINTLN("Info: Logging Terminated");   
      closeGNSS(); 
      logDebug("success");                 
      configureSleepAlarm();
      deinitializeBuses();
    }
    
    petDog();

    if (ledBlink) {                  // Only if User wants blinking every WDT interrupt
      blinkLed(1, 100);      
    }
 
    goToSleep();  
}

///////// INTERRUPT HANDLERS
// Watchdog
extern "C" void am_watchdog_isr(void) {
  
  wdt.clear();      // Clear the watchdog interrupt

  if (wdtCounter < 5) { 
    wdt.restart();  // Restart the watchdog timer
  }                 // ELSE, if WDT will keep counting until it reaches RESET period
  
  wdtFlag = true;   // Set the watchdog flag
  wdtCounter++;     // Increment watchdog interrupt counter

  if (wdtCounter > wdtCounterMax) {
    wdtCounterMax = wdtCounter;
  }
}

//// RTC
extern "C" void am_rtc_isr(void) {
  am_hal_rtc_int_clear(AM_HAL_RTC_INT_ALM); // Clear the RTC alarm interrupt
  alarmFlag = true;                         // Set RTC flag
}
