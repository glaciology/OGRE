/*
   OGRENet: On-ice GNSS Research Experimental Network for Greenland
   Derek Pickell 12/28/21
   V0.2.0 (beta-release)

   Hardware:
   - Artemis MicroMod v.???
   - Ublox ZED-F9P-01B-01

   Dependencies:
   - SparkFun_u-blox_GNSS_Arduino_Library v2.0.5
   - Apollo3 Arduino Core v1.2.3
   - SdFat v2.0.6

   INSTRUCTIONS:
   - Enter user defined variables in "USER DEFINED" Section below
   - LED indicators:
        *1 Blink every 10 seconds: Sleeping 
        *10 Blinks: System Configuration Complete (After Initial Power On or Reset Only)

   Issues: 
   - Peripheral power: https://issueexplorer.com/issue/sparkfun/Arduino_Apollo3/407
   - SD CARD initialization: https://github.com/sparkfun/Arduino_Apollo3/issues/370

   TODO: 
   - RTC scheme - actual time
   - Testing: SD card to large size, millis rollover
   - PERIPHERAL PWR control
   - Update Sparkfun GNSS, Artemis, SDFat libraries to newer versions?
   - HOTSTART UBX-SOS
   - DEBUG file: failures, online/offline, timers, LED issues
   - File scheme - naming or appending
*/

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
//////////////////

///////// PINOUTS
#define LED                 LED_BUILTIN
#define ZED_POWER             34  // G2 - Drive low to turn off ZED
#define PERIPHERAL_POWER      33  // G1 - Drive low to turn off SD/Perhipherals
#define PIN_SD_CS             41  // CS
#define SDA                   44
#define SCL                   45
#define SD_CONFIG             SdSpiConfig(PIN_SD_CS, SHARED_SPI, SD_SCK_MHZ(24))
//////////////////

//////////////////////////////////////////////////////
//----------- USERS SPECIFY INTERVAL HERE ------------
// LOG MODE: ROLLING OR DAILY
byte logMode                = 2;   // 1 = daily, 2 = rolling

// LOG MODE: Rolling Interval
uint32_t secondsSleep       = 20;  // SLEEP INTERVAL (S)
uint32_t minutesSleep       = 0;
uint32_t secondsLog         = 30;  // LOGGING INTERVAL (S)
uint32_t minutesLog         = 0;

// LOG MODE: Daily Interval **** NOT FULLY CONFIGURED YET
byte logStartHr             = 10;  // UTC
byte logEndHr               = 14;  // UTC

// UBLOX Message Config:
int logGPS               = 1;      // FOR EACH CONSTELLATION 1 = ENABLE, 0 = DISABLE
int logGLO               = 1;
int logGAL               = 0;
int logBDS               = 0;
int logQZSS              = 0;
bool logNav              = true;   // TRUE if SFRBX (satellite nav) included in logging
//----------------------------------------------------
//////////////////////////////////////////////////////

///////// GLOBAL VARIABLES
const int apolloCore       = 1;     // VERSION 1 or 2...
const int sdWriteSize      = 512;   // Write data to SD in blocks of 512 bytes
const int fileBufferSize   = 16384; // Allocate 16KB RAM for UBX message storage
volatile bool firstConfig  = true;  // First sys run will configure specific settings in GNSS, then false
volatile bool wdtFlag      = false; // ISR WatchDog
volatile int  wdtCounter   = 0;    
volatile int wdtCounterMax = 0;
volatile bool stimerFlag   = false; //
volatile bool alarmFlag    = true;  // RTC alarm true when interrupt (initialized as true for first loop)
volatile bool initSetup    = true;  // 
unsigned long bytesWritten = 0;     // used for printing to Serial Monitor bytes written to SD
unsigned long lastPrint    = 0;     // used to printing bytesWritten to Serial Monitor at 1Hz
unsigned long prevMillis   = 0;
unsigned long currMillis   = 0; 
//////////////////

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
    Serial.println("OGRENet GNSS LOGGER");
  #endif

  ///////// CONFIGURE INITIAL SETTINGS
  pinMode(ZED_POWER, OUTPUT);
  pinMode(PERIPHERAL_POWER, OUTPUT);
  pinMode(LED, OUTPUT);

  qwiicPowerOn();
  peripheralPowerOn();
  
  Wire.begin();
  disableI2CPullups();
  SPI.begin();

  configureWdt(); 
}

void loop() {
  
    if (alarmFlag) { 
      DEBUG_PRINTLN("Alarm Triggered: Configuring System");     
      petDog();  
      qwiicPowerOn();      // TURN UBLOX ON
      peripheralPowerOn(); // TURN SD & PERIPHERALS ON
      delay(100); // delay(1000);
      configureSD();       // CONFIGURE SD
      configureGNSS();     // CONFIGURE GNSS SETTINGS
      configureLogAlarm(); // sets RTC clock to interrupt log end
      DEBUG_PRINTLN("Configuration Complete"); 
      
      if (initSetup){      // Indicates Config Complete 
        blinkLed(10, 100);
        initSetup = false;
      }
      
      while(!alarmFlag) {  // LOG DATA UNTIL alarmFlag = True
        logGNSS();
      }
      
      DEBUG_PRINTLN("Logging Terminated: Sleep");   
      closeGNSS(); 
      configureSleepAlarm();
    }

    if (wdtFlag) {
      petDog(); // Restart watchdog timer
    }
    
    //DEBUG_PRINTLN(wdtCounterMax);
    
    blinkLed(1, 100);
    goToSleep();
  
}

///////// INTERRUPT HANDLERS

// Watchdog
extern "C" void am_watchdog_isr(void){

  // Clear WDT interrupt
  am_hal_wdt_int_clear();

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
extern "C" void am_rtc_isr(void)
{
  // Clear the RTC alarm interrupt
  am_hal_rtc_int_clear(AM_HAL_RTC_INT_ALM);

  // Set alarm flag
  alarmFlag = true;
}
