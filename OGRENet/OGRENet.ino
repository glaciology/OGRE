/*
   OGRENet: On-ice GNSS Research Experimental Network for Greenland
   By: Derek Pickell
   V0.0.1 (pre-release)

   Hardware:
   - Artemis MicroMod v.???
   - Ublox ZED-F9P-01B-01

   Dependencies:
   - SparkFun_u-blox_GNSS_Arduino_Library v2.0.5
   - Apollo3 Arduino Core v2.1.0
   - SdFat v2.0.6

   Issues: 
   - Peripheral power: https://issueexplorer.com/issue/sparkfun/Arduino_Apollo3/407
*/

///////// LIBRARIES & OBJECT INSTANTIATION(S)
#include <Wire.h> // SDA 44 SCL 45 
#include <SdFat.h>   // 
#include <SPI.h>  // CS 41 MISO/MOSI/CLK ? 
#include <RTC.h>  //
#include <SparkFun_u-blox_GNSS_Arduino_Library.h> // Library v2.0.5: http://librarymanager/All#SparkFun_u-blox_GNSS
SFE_UBLOX_GNSS myGNSS;
SdFs sd;
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
#define PIN_PERIPHERAL      33  // G1 - Drive low to turn of SD/Perhipherals
//////////////////

///////// TIMING - SPECIFY INTERVAL HERE -  -------------------------
uint32_t msToSleep        = 50000;   // SLEEP INTERVAL (MS)
uint32_t secondsLog       = 5;      // LOGGING INTERVAL (S)
//-------------------------------------------------------------------
#define TIMER_FREQ 32768L // CTimer6 will use the 32kHz clock
uint32_t sysTicksToSleep = msToSleep * TIMER_FREQ / 1000;
//////////////////

///////// GLOBAL VARIABLES
unsigned long bytesWritten = 0;
unsigned long lastPrint    = 0; // used to keep time for logging

volatile bool wdtFlag      = false; 
volatile int  wdtCounter   = 0; 
volatile int wdtCounterMax = 0;

volatile bool stimerFlag   = false;
volatile bool alarmFlag    = true;

//////////////////

///////// DEBUGGING
#define DEBUG true // Output messages to Serial monitor
#if DEBUG
#define DEBUG_PRINTLN(x)  Serial.println(x)
#define DEBUG_SERIALFLUSH() Serial.flush()
#else
#define DEBUG_PRINTLN(x)
#define DEBUG_SERIALFLUSH()
#endif
//////////////////

void setup(void) {
  #if DEBUG
    Serial.begin(115200); // open serial port
    Serial.println("OGRENet GNSS LOGGER");
  #endif
   
  Wire.begin();
  SPI.begin();
  pinMode(LED, OUTPUT);
  //pinMode(PIN_ZED_POWER, OUTPUT);
  //pinMode(PIN_PERIPHERAL, OUTPUT);

  disableI2CPullups();

  #if DEBUG
    while (Serial.available()) {
      Serial.read();
    }
  #endif

  ///////// CONFIGURE SETTINGS
//  configureSD();
//  configureGNSS();
  configureWdt();
  //configureLogAlarm();
  DEBUG_PRINTLN("Configuration Complete");
  /////////

}

void loop(void) {
  
    if (alarmFlag){
      configureLogAlarm();
      configureSD();
      configureGNSS();
  
      while(!alarmFlag){
        logGNSS();
      }
      closeGNSS();
    }
    
    ///////// SLEEP SHUTDOWN PREP
    //alarmFlag = false;
    DEBUG_PRINTLN("timeout, going to sleep");
    delay(100);
    myFile.close();
    bytesWritten = 0;
    lastPrint = 0;
    DEBUG_SERIALFLUSH();
    delay(100);


    if (wdtFlag) {
      petDog(); // Restart watchdog timer
    }
    
    blinkLed(1, 100);
    goToSleep();

//  if (alarmFlag) { ///////// FINISH LOGGING
//    DEBUG_PRINTLN("alarm triggered");
//
//    closeGNSS();
//    
//    ///////// SLEEP SHUTDOWN PREP
//    alarmFlag = false;
//    DEBUG_PRINTLN("timeout, going to sleep");
//    delay(100);
//    myFile.close();
//    bytesWritten = 0;
//    lastPrint = 0;
//    DEBUG_SERIALFLUSH();
//    delay(100);
//    goToSleep();
//    /////////
//  }
//
//  logGNSS();
//   pinMode(LED, HIGH);
//  if (!wdtFlag) {
//    
//  }
  
//  if (wdtFlag) {
//    petDog(); // Restart watchdog timer
//    //blinkLed(1, 100);
//    //goToSleep();
//  }
  
}

///////// Interrupt handler for STIMER
extern "C" void am_stimer_cmpr6_isr(void){
  uint32_t ui32Status = am_hal_stimer_int_status_get(false);
  
  if (ui32Status & AM_HAL_STIMER_INT_COMPAREG)
  {
    am_hal_stimer_int_clear(AM_HAL_STIMER_INT_COMPAREG); // 
  }

  stimerFlag = true;
}

///////// Interrupt handler for the watchdog.
extern "C" void am_watchdog_isr(void){
  am_hal_wdt_int_clear();

  if ( wdtCounter < 10 ){  // Catch the first three watchdog interrupts, but let the fourth through untouched.
    am_hal_wdt_restart(); // "Pet" the dog (reset the timer)
  }
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
  //rtc.clearInterrupt();
  am_hal_rtc_int_clear(AM_HAL_RTC_INT_ALM);

  // Set alarm flag
  alarmFlag = true;
}
