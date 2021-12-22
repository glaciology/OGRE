/*
   OGRENet: On-ice GNSS Research Experimental Network for Greenland
   By: Derek Pickell
   V0.1.0 (beta-release)

   Hardware:
   - Artemis MicroMod v.???
   - Ublox ZED-F9P-01B-01

   Dependencies:
   - SparkFun_u-blox_GNSS_Arduino_Library v2.0.5
   - Apollo3 Arduino Core v2.1.0
   - SdFat v2.0.6

   Instructions:
   - User defined variables in "USER DEFINED" Section below
   - LED indicators:

   Issues: 
   - Peripheral power: https://issueexplorer.com/issue/sparkfun/Arduino_Apollo3/407
   - SD CARD initialization: https://github.com/sparkfun/Arduino_Apollo3/issues/370

   TODO: 
   - LED scheme - blinks
   - RTC scheme - actual time
   - Test SD card to large size
   - PERIPHERAL PWR control
   - Update Sparkfun GNSS, Artemis, SDFat libraries to newer versions?
   - HOTSTART UBX-SOS
   - DEBUG file ?
   
*/

///////// LIBRARIES & OBJECT INSTANTIATIONS
#include <Wire.h>                                  // SDA 44 SCL 45 
#include <SdFat.h>                                 // https://github.com/greiman/SdFat v2.0.6
#include <SPI.h>                                   // CS 41 MISO/MOSI/CLK ? 
#include <RTC.h>                                   //
#include <SparkFun_u-blox_GNSS_Arduino_Library.h>  // Library v2.0.5: http://librarymanager/All#SparkFun_u-blox_GNSS
SFE_UBLOX_GNSS myGNSS;                             //
SdFs sd;                                           // SdFs = FAT16/FAT32 and exFAT (4GB+)
//////////////////

///////// SD CARD & BUFFER SIZES
File myFile;                        // File that all GNSS data is written to
#define sdWriteSize         512     // Write data to the SD card in blocks of 512 bytes
#define fileBufferSize      16384   // Allocate 16KBytes of RAM for UBX message storage
//////////////////

///////// PINOUTS
#define LED                 LED_BUILTIN
#define PIN_ZED_POWER       34  // G2 - Drive low to turn off ZED
#define PIN_SD_CS           41  // CS
#define PIN_PERIPHERAL      33  // G1 - Drive low to turn off SD/Perhipherals
//////////////////

///////// USERS SPECIFY INTERVAL HERE -  -------------------------
//uint32_t msToSleep        = 50000;   // SLEEP INTERVAL (MS)
uint32_t secondsSleep       = 59;
uint32_t secondsLog         = 59;      // LOGGING INTERVAL (S)
//-------------------------------------------------------------------
//#define TIMER_FREQ          32768L // CTimer6 will use the 32kHz clock
//uint32_t sysTicksToSleep  = msToSleep * TIMER_FREQ / 1000;

int logGPS               = 1;    // FOR EACH CONSTELLATION 1 = ENABLE, 0 = DISABLE
int logGLO               = 1;
int logGAL               = 0;
int logBDS               = 0;
int logQZSS              = 0;

bool logNav              = true; // TRUE if SFRBX (satellite nav) included in logging
////////////////// 

///////// GLOBAL VARIABLES
volatile bool firstConfig  = true;  // first time system runs, will configure specific settings in GNSS
unsigned long bytesWritten = 0;     // used for printing to Serial Monitor bytes written to SD
unsigned long lastPrint    = 0;     // used to printing bytesWritten to Serial Monitor at 1Hz

volatile bool wdtFlag      = false; // if WatchDog triggers interrupt, becomes true
volatile int  wdtCounter   = 0;    
volatile int wdtCounterMax = 0;

volatile bool stimerFlag   = false; //
volatile bool alarmFlag    = true;  // RTC alarm true when interrupt (initialized as true for first loop)
//////////////////

///////// DEBUGGING
#define DEBUG true // Output messages to Serial monitor
#if DEBUG
#define DEBUG_PRINTLN(x)    Serial.println(x)
#define DEBUG_SERIALFLUSH() Serial.flush()

#else
#define DEBUG_PRINTLN(x)
#define DEBUG_SERIALFLUSH()
#endif
//////////////////

void setup(void) {
  #if DEBUG
    Serial.begin(115200); // open serial port at 115200 baud
    Serial.println("OGRENet GNSS LOGGER");
    while (Serial.available()) {
      Serial.read();
    }
  #endif

  ///////// CONFIGURE INITIAL SETTINGS
  Wire.begin();
  disableI2CPullups();
  SPI.begin();
  pinMode(LED, OUTPUT);
  //qwiicPowerOff();
  configureWdt(); 
  /////////
  
  //pinMode(PIN_ZED_POWER, OUTPUT);
  //pinMode(PIN_PERIPHERAL, OUTPUT);

}

void loop(void) {
  
    if (alarmFlag){ 
      DEBUG_PRINTLN("Log BEGIN Triggered");     
      petDog();  
      configureLogAlarm(); // sets RTC clock interrupt "alarmFlag"= true when interrupt 
      qwiicPowerOn();
      delay(1000);
      configureSD();       
      configureGNSS();
  
      while(!alarmFlag){
        logGNSS();
      }
      DEBUG_PRINTLN("Log END Triggered, Sleep");   
      closeGNSS(); 
      configureSleepAlarm();
    }

    if (wdtFlag) {
      petDog(); // Restart watchdog timer
    }
    
    blinkLed(1, 100);
    DEBUG_PRINTLN("SLEEP");
    DEBUG_SERIALFLUSH();
    goToSleep();
  
}

///////// Interrupt handler for STIMER
//extern "C" void am_stimer_cmpr6_isr(void){
//  uint32_t ui32Status = am_hal_stimer_int_status_get(false);
//  
//  if (ui32Status & AM_HAL_STIMER_INT_COMPAREG)
//  {
//    am_hal_stimer_int_clear(AM_HAL_STIMER_INT_COMPAREG); // 
//  }
//
//  stimerFlag = true;
//}

///////// Interrupt handler for the watchdog.
extern "C" void am_watchdog_isr(void){

  // Clear WDT interrupt
  am_hal_wdt_int_clear();

//  if ( wdtCounter < 10 ){  
//    am_hal_wdt_restart(); // "Pet" the dog (reset the timer)
//  }

  DEBUG_PRINTLN("WDT Trigger");
  wdtFlag = true; // Set the watchdog flag
  wdtCounter++; // Increment watchdog interrupt counter
  
  if (wdtCounter > wdtCounterMax){
    wdtCounterMax = wdtCounter;
  }
}

///////// Interrupt handler for the RTC
extern "C" void am_rtc_isr(void)
{
  // Clear the RTC alarm interrupt
  am_hal_rtc_int_clear(AM_HAL_RTC_INT_ALM);

  // Set alarm flag
  alarmFlag = true;
}
