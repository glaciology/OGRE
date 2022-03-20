/*
   OGRENet: On-ice GNSS Research Experimental Network for Greenland
   Derek Pickell 1/19/22
   V0.?.0 (beta-release)

   Hardware:
   - OGRENet PCB or Sparkfun Artemis & Ublox ZED-F9P

   Dependencies:
   - SparkFun_u-blox_GNSS_Arduino_Library v2.0.5
   - Apollo3 Arduino Core v1.2.3
   - SdFat Library v2.0.6

   INSTRUCTIONS:
   - Enter user defined variables in "USER DEFINED" Section below
   - LED indicators:
        *2 Blink Pattern: uSD failed
        *3 Blink Pattern: Ublox failed
        *5 Rapid Blinks: RTC sync fail
        *10 Blinks: RTC synced and System Configuration Complete (After Initial Power On or Reset Only)
        *1 Blink every 12 seconds: Sleeping 

   TODO: 
   - HOTSTART UBX-SOS?
*/
#define HARDWARE_VERSION 0    // 0 = CUSTOM DARTMOUTH HARDWARE, 1 = SPARKFUN ARTEMIS MICROMOD DATA LOGGER 
#define SOFTWARE_VERSION 0.9  // print this to determine which version of software used on device

///////// LIBRARIES & OBJECT INSTANTIATIONS
#include <Wire.h>                                  // 
#include <WDT.h>                                   //
#include <RTC.h>                                   //
#include <SdFat.h>                                 // https://github.com/greiman/SdFat v2.0.6
#include <SPI.h>                                   // 
#include <SparkFun_u-blox_GNSS_Arduino_Library.h>  // Library v2.0.5: http://librarymanager/All#SparkFun_u-blox_GNSS
SFE_UBLOX_GNSS gnss;                               //
SdFs sd;                                           // SdFs = supports FAT16, FAT32 and exFAT (4GB+), corresponding to FsFile class
APM3_RTC rtc;                                      //
APM3_WDT wdt;                                      //
FsFile myFile;                                     // RAW UBX LOG FILE
FsFile debugFile;                                  // DEBUG LOG FILE
FsFile configFile;                                 // USER INPUT CONFIG FILE

///////// HARDWARE SPECIFIC PINOUTS & OBJECTS
#if HARDWARE_VERSION == 0
const byte LED                    = 33;  //
const byte PER_POWER              = 18;  // Drive to turn off uSD
#elif HARDWARE_VERSION == 1
const byte LED                    = 19;  //
const byte PER_POWER              = 33;  // Drive to turn off uSD
#endif
const byte ZED_POWER              = 34;  // Drive to turn off ZED
const byte PIN_SD_CS              = 41;  //
const byte BAT                    = 32;  // ADC port for battery measure
const byte BAT_CNTRL              = 22;  // Drive high to turn on Bat measure

#if HARDWARE_VERSION == 0
TwoWire myWire(2);                       // USE I2C bus 2, SDA/SCL 25/27
#elif HARDWARE_VERSION == 1              //
TwoWire myWire(4);                       // USE I2C bus 4, 39/40
#endif

SPIClass mySpi(3);                       // Use SPI 3
#define SD_CONFIG SdSpiConfig(PIN_SD_CS, SHARED_SPI, SD_SCK_MHZ(24), &mySpi)

//////////////////////////////////////////////////////
//----------- DEFAULT CONFIGURATION HERE ------------
// LOG MODE: ROLLING OR DAILY
byte logMode                = 4;        // 1 = daily fixed, 2 = continous, 3 = monthly , 4 = test mode

// LOG MODE 1: DAILY, DURING DEFINED HOURS
byte logStartHr             = 16;       // UTC Hour 
byte logEndHr               = 20;       // UTC Hour

// LOG MODE 3: ONCE/MONTH FOR 24 HOURS
byte logStartDay            = 1;        // Day of month between 1 and 28

// LOG MODE 4: TEST: ALTERNATE SLEEP/LOG FOR X SECONDS
uint32_t secondsSleep       = 50;       // SLEEP INTERVAL (Seconds)
uint32_t secondsLog         = 50;       // LOGGING INTERVAL (Seconds)

// UBLOX MESSAGE CONFIGURATION: 
int logGPS                  = 1;        // FOR EACH CONSTELLATION 1 = ENABLE, 0 = DISABLE
int logGLO                  = 1;
int logGAL                  = 0;
int logBDS                  = 0;
int logQZSS                 = 0;
int logNav                  = 1;     

// ADDITIONAL CONFIGURATION
bool ledBlink               = true;     // If FALSE, all LED indicators during log/sleep disabled
bool measureBattery         = true;    // If TRUE, uses battery circuit to measure V during debug logs
//----------------------------------------------------
//////////////////////////////////////////////////////

///////// GLOBAL VARIABLES
const int     sdWriteSize         = 512;      // Write data to SD in blocks of 512 bytes
const int     fileBufferSize      = 16384;    // Allocate 16KB RAM for UBX message storage
volatile bool wdtFlag             = false;    // ISR WatchDog
volatile bool rtcSyncFlag         = false;    // Flag to indicate if RTC has been synced with GNSS
volatile bool alarmFlag           = true;     // RTC alarm true when interrupt (initialized as true for first loop)
volatile bool initSetup           = true;     // False once GNSS messages configured-will not configure again
unsigned long prevMillis          = 0;        // Global time keeper, not affected by Millis rollover
int           settings[15]        = {};       // Array that holds USER settings on SD
char line[25];                                // Temporary array for parsing USER settings
int stationName                   = 1111;     // Station name, 4 digits
char logFileName[12]              = "";
char logFileNameDate[30]          = ""; // Log file name


// DEBUGGING
uint16_t      maxBufferBytes      = 0;        // How full the file buffer has been
unsigned long bytesWritten        = 0;        // used for printing to Serial Monitor bytes written to SD
unsigned long syncFailCounter     = 0;        // Counts RTC sync failures
unsigned long writeFailCounter    = 0;        // Counts uSD write failures
unsigned long closeFailCounter    = 0;        // Counts uSD close failures
unsigned int  debugCounter        = 0;        // Counts Debug messages
volatile int  wdtCounter          = 0;        // Counts WDT triggers
volatile int  wdtCounterMax       = 0;        // Tracks Max times WDT interrupts
long          rtcDrift            = 0;        // Tracks drift of RTC

struct struct_online {
  bool uSD      = false;
  bool gnss     = false;
  bool logGnss  = false;
} online;
//////////////////////////////////////////////////////

///////// DEBUGGING MACROS
#define DEBUG                     false  // Output messages to Serial monitor
#define DEBUG_GNSS                false  // Output GNSS debug messages to Serial monitor

#if DEBUG
#define DEBUG_PRINTLN(x)    Serial.println(x)
#define DEBUG_PRINT(x)      Serial.print(x)

#else
#define DEBUG_PRINTLN(x)
#define DEBUG_PRINT(x)

#endif
//////////////////////////////////////////////////////


void setup() {
  
  #if DEBUG
    Serial.begin(115200);
    delay(1000);
    Serial.println("***WELCOME TO GNSS LOGGER***");
  #endif

  ///////// CONFIGURE INITIAL SETTINGS
  initializeBuses();                 // Initializes I2C & SPI and turns on ZED (I2C), uSD (SPI)
  pinMode(LED, OUTPUT);              //
  configureWdt();                    // 12s interrupt, 24s reset period
  configureSD();                     // BLINK 2x pattern - FAILED SETUP
  getConfig();                       // Read LOG settings from Config.txt on uSD
  configureGNSS();                   // BLINK 3x pattern - FAILED SETUP
  createDebugFile();                 //
  syncRtc();                         // 1Hz BLINK-AQUIRING; 5x - FAIL (3 min MAX)

  if (logMode == 1 || logMode == 3){ // GET GPS TIME if MONTHLY or DAILY logging
//       syncRtc();                    
       configureSleepAlarm();
       DEBUG_PRINT("Info: Sleeping until: "); printAlarm();
       deinitializeBuses();
  }

  blinkLed(10, 100);                 // BLINK 10x - SETUP COMPLETE
  DEBUG_PRINTLN("Info: SETUP COMPLETE");
}


void loop() {
  
    if (alarmFlag) {                // SLEEP UNTIL alarmFlag = True
      DEBUG_PRINTLN("Info: Alarm Triggered: Configuring System");
      petDog();

      initializeBuses();            // CONFIGURE I2C, SPI, ZED, uSD
      configureSD();                // CONFIGURE SD
      configureGNSS();              // CONFIGURE GNSS SETTINGS

      if (logMode == 1 || logMode == 3) {
        syncRtc();                  // SYNC RTC W/ GPS (3 min MAX)
      }

      configureLogAlarm();          // RTC INTERRUPT WHEN DONE LOGGING
      DEBUG_PRINT("Info: Logging until "); printAlarm();
      
      while(!alarmFlag) {           // LOG DATA UNTIL alarmFlag = True
        petDog();
        logGNSS();
      }
      
      DEBUG_PRINTLN("Info: Logging Terminated");   
      closeGNSS(); 
      logDebug();                 
      configureSleepAlarm();
      deinitializeBuses();
      
      DEBUG_PRINT("Info: Sleeping until "); printAlarm();
    }

    if (wdtFlag) {                 // IF WDT interrupt, restart timer by petting dog
      petDog(); 
    }  

    if (ledBlink) {               // Only if User wants blinking
      blinkLed(1, 100);           // BLINK once every WDT trigger
    }
 
    goToSleep();  
}

///////// INTERRUPT HANDLERS
// Watchdog
extern "C" void am_watchdog_isr(void) {
  
  wdt.clear();      // Clear the watchdog interrupt

  if (wdtCounter < 10) { 
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
  alarmFlag = true; // Set RTC flag
}