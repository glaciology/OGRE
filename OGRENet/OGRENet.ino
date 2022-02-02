/*
   OGRENet: On-ice GNSS Research Experimental Network for Greenland
   Derek Pickell 1/19/22
   V0.2.0 (beta-release)

   Hardware:
   - Sparkfun Artemis
   - Ublox ZED-F9P

   Dependencies:
   - SparkFun_u-blox_GNSS_Arduino_Library v2.0.5
   - Apollo3 Arduino Core v1.2.3
   - SdFat v2.0.6

   INSTRUCTIONS:
   - Enter user defined variables in "USER DEFINED" Section below
   - LED indicators:
        *1 Blink every 10 seconds: Sleeping 
        *10 Blinks: System Configuration Complete (After Initial Power On or Reset Only)

   TODO: 
   - Testing: SD card to large size, millis rollover
   - HOTSTART UBX-SOS
   - File scheme - naming or appending??
   - Continous mode: make sure data saves when unplugged
*/
const int HARDWARE_VERSION  = 0; // 0 = CUSTOM DARTMOUTH HARDWARE, 1 = SPARKFUN MICROMOD

///////// LIBRARIES & OBJECT INSTANTIATIONS
#include <Wire.h>                                  // SDA 44 SCL 45 
#include <WDT.h>
#include <RTC.h>                                   //
#include <SdFat.h>                                 // https://github.com/greiman/SdFat v2.0.6
#include <SPI.h>                                   // CS 41 MISO/MOSI/CLK ? 
#include <SparkFun_u-blox_GNSS_Arduino_Library.h>  // Library v2.0.5: http://librarymanager/All#SparkFun_u-blox_GNSS
SFE_UBLOX_GNSS gnss;                               //
SdFs sd;                                           // SdFs = FAT16/FAT32 and exFAT (4GB+)
APM3_RTC rtc;
APM3_WDT wdt;
File myFile;
FsFile debugFile;
//////////////////

///////// PINOUTS & OBJ INSTANCES
#if(HARDWARE_VERSION ==0)
TwoWire myWire(2);                  // USE I2C bus 2, SDA/SCL 25/27
SPIClass mySpi(3);                  // Use IO Master 4 on pads 39/40/38
#define LED                     16
#define ZED_POWER               34  // Drive low to turn off ZED
#define PER_POWER               18  // Drive low to turn off uSD
#define PIN_SD_CS               41  
#elif
TwoWire myWire();                  // USE default I2C
SPIClass mySpi();                  // Use default SPI
#define LED                     LED_BUILTIN
#define ZED_POWER               0  // Drive low to turn off ZED
#define PER_POWER               0  // Drive low to turn off uSD
#define PIN_SD_CS               0  
#endif
#define SD_CONFIG               SdSpiConfig(PIN_SD_CS, DEDICATED_SPI, SD_SCK_MHZ(24), &mySpi)
//////////////////

//////////////////////////////////////////////////////
//----------- USERS SPECIFY INTERVAL HERE ------------
// LOG MODE: ROLLING OR DAILY
byte logMode                = 2;   // 1 = daily fixed, 2 = daily rolling, 3 continuous, 4 monthly

// LOG MODE 1: DAILY, DURING DEFINED HOURS
byte logStartHr             = 10;  // UTC
byte logEndHr               = 12;  // UTC

// LOG MODE 2: TEST: ALTERNATE SLEEP/LOG FOR X SECONDS
uint32_t secondsSleep       = 50;  // SLEEP INTERVAL (S)
uint32_t minutesSleep       = 0;
uint32_t secondsLog         = 50;  // LOGGING INTERVAL (S)
uint32_t minutesLog         = 0;

// LOG MODE 4: ONCE/MONTH FOR 24 HOURS
byte logStartDay            = 1;  // Day of month between 1 and 28

// UBLOX MESSAGE CONFIGURATION: 
int logGPS                  = 1;    // FOR EACH CONSTELLATION 1 = ENABLE, 0 = DISABLE
int logGLO                  = 1;
int logGAL                  = 0;
int logBDS                  = 0;
int logQZSS                 = 0;
bool logNav                 = true; // TRUE if SFRBX (satellite nav) included in logging
//----------------------------------------------------
//////////////////////////////////////////////////////

///////// GLOBAL VARIABLES
const int apolloCore              = 1;     // VERSION 1 or 2...
const int sdWriteSize             = 512;   // Write data to SD in blocks of 512 bytes
const int fileBufferSize          = 16384; // Allocate 16KB RAM for UBX message storage
uint16_t  maxBufferBytes          = 0;     // How full the file buffer has been
//volatile bool firstConfig         = true;  // First sys run will configure specific settings in GNSS, then false
volatile bool wdtFlag             = false; // ISR WatchDog
volatile bool rtcSyncFlag         = false; // Flag to indicate if RTC has been synced with GNSS
volatile bool stimerFlag          = false; //
volatile bool alarmFlag           = true;  // RTC alarm true when interrupt (initialized as true for first loop)
volatile bool initSetup           = true;  // 
unsigned long bytesWritten        = 0;     // used for printing to Serial Monitor bytes written to SD
unsigned long lastPrint           = 0;     // used to printing bytesWritten to Serial Monitor at 1Hz
unsigned long prevMillis          = 0;
unsigned long currMillis          = 0; 
// DEBUGGING
unsigned long syncFailCounter     = 0;
unsigned long writeFailCounter    = 0;
unsigned long closeFailCounter    = 0;
unsigned int  debugCounter        = 0;     // Counter to track number of recorded debug messages
volatile int  wdtCounter          = 0;    
volatile int  wdtCounterMax       = 0;
long          rtcDrift            = 0;     // Counter for drift of RTC
//////////////////

///////// GLOBAL VARIABLES
struct struct_online
{
  bool uSD  = false;
  bool gnss     = false;
  bool logGnss  = false;
  bool logDebug = false;
} online;

///////// DEBUGGING
#define DEBUG               false // Output messages to Serial monitor
#if DEBUG
#define DEBUG_PRINTLN(x)    Serial.println(x)
#define DEBUG_SERIALFLUSH() Serial.flush()
#define PROG_TIMER()        millis()

#else
#define DEBUG_PRINTLN(x)
#define DEBUG_SERIALFLUSH()
#define PROG_TIMER()
#endif
//////////////////

void setup() {
  #if DEBUG
    Serial.begin(115200); // open serial port at 115200 baud
    delay(1000);
    Serial.println("***WELCOME TO GNSS LOGGER***");
  #endif

  ///////// CONFIGURE INITIAL SETTINGS
  pinMode(ZED_POWER, OUTPUT);
  pinMode(PER_POWER, OUTPUT);
  pinMode(LED, OUTPUT);

  zedPowerOn();
  peripheralPowerOn();
  myWire.begin();
  disableI2CPullups();
  mySpi.begin();
  configureWdt();      
  configureSD();          
  configureGNSS();
  createDebugFile();        

  if (logMode == 1 || logMode == 4){ // GET GPS TIME
       syncRtc();
       configureSleepAlarm();
       initSetup = false;
       DEBUG_PRINTLN("Info: Datetime "); printDateTime();
  }

  blinkLed(10, 100); // BLINK 10x INDICATE SETUP COMPLETE
  DEBUG_PRINTLN("SETUP LOOP COMPLETE");
}

void loop() {
  
    if (alarmFlag) { 
      DEBUG_PRINTLN("Alarm Triggered: Configuring System");     
      petDog();  
      zedPowerOn();           // TURN UBLOX ON
      peripheralPowerOn();    // TURN SD & PERIPHERALS ON
      delay(100);             // delay(1000);
      configureSD();          // CONFIGURE SD
      configureGNSS();        // CONFIGURE GNSS SETTINGS
      
      if (logMode == 1 || logMode == 4){
        syncRtc();
      }
     
      configureLogAlarm();    // sets RTC clock to interrupt log end
      
      while(!alarmFlag) {     // LOG DATA UNTIL alarmFlag = True
        petDog();
        logGNSS();
      }
      
      DEBUG_PRINTLN("Logging Terminated: Sleep");   
      closeGNSS(); 
      configureSleepAlarm();
    }

    if (wdtFlag) {
      petDog(); 
    }
    
    //DEBUG_PRINTLN(wdtCounterMax);
    
    blinkLed(1, 100); // Blinks once every WDT trigger
    goToSleep();
  
}

///////// INTERRUPT HANDLERS

// Watchdog
extern "C" void am_watchdog_isr(void) {

  am_hal_wdt_int_clear(); // Clear WDT interrupt

  if ( wdtCounter < 10 ){  
    am_hal_wdt_restart(); // "Pet" the dog (reset the timer)
  }

  DEBUG_PRINTLN("WDT Trigger");
  wdtFlag = true; // Set the watchdog flag
  wdtCounter++; // Increment watchdog interrupt counter
  
  if (wdtCounter > wdtCounterMax){
    wdtCounterMax = wdtCounter;
  }
}

//// RTC
extern "C" void am_rtc_isr(void) {
  am_hal_rtc_int_clear(AM_HAL_RTC_INT_ALM); // Clear the RTC alarm interrupt
  alarmFlag = true; // Set RTC flag
}
