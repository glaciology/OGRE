// Write Lots of stuff to sd card

///////// LIBRARIES & OBJECT INSTANTIATIONS //////////
#include <Wire.h>                             // 
#include <SPI.h>                              // 
#include <WDT.h>                              //
#include <RTC.h>                              //
#include <time.h>                             //
#include <SdFat.h>                            // https://github.com/greiman/SdFat v2.1.0
#include <SparkFun_u-blox_GNSS_Arduino_Library.h>  // Library v2.2.8: http://librarymanager/All#SparkFun_u-blox_GNSS
SFE_UBLOX_GNSS gnss;                          //
SdFs sd;                                      // SdFs = supports FAT16, FAT32 and exFAT (4GB+), corresponding to FsFile class
APM3_RTC rtc;                                 //
APM3_WDT wdt;                                 // 
FsFile myFile;                                // RAW UBX LOG FILE
FsFile debugFile;                             // DEBUG LOG FILE
FsFile configFile;                            // USER INPUT CONFIG FILE
FsFile dateFile;                              // USER INPUT EPOCHS FILE

///////// HARDWARE-SPECIFIC PINOUTS & OBJECTS ////////
#if HARDWARE_VERSION == 0
const byte BAT                    = 32;       // ADC port for battery measure
#elif HARDWARE_VERSION == 1                   //
const byte BAT                    = 35;       //
#endif                                        //
const byte PER_POWER              = 18;       // Drive to turn off uSD
const byte ZED_POWER              = 34;       // Drive to turn off ZED
const byte LED                    = 33;       //
const byte PIN_SD_CS              = 41;       //
const byte BAT_CNTRL              = 22;       // Drive high to turn on Bat measure

TwoWire myWire(2);                            // USE I2C bus 2, SDA/SCL 25/27
SPIClass mySpi(3);                            // Use SPI 3 - pins 38, 41, 42, 43
#define SD_CONFIG SdSpiConfig(PIN_SD_CS, DEDICATED_SPI, SD_SCK_MHZ(24), &mySpi)

///////// GLOBAL VARIABLES ///////////////////////////
const int     sdWriteSize         = 512;      // Write data to SD in blocks of 512 bytes
const int     fileBufferSize      = 16384;    // Allocate 16KB RAM for UBX message storage
volatile bool wdtFlag             = false;    // ISR WatchDog
volatile bool rtcSyncFlag         = false;    // Flag to indicate if RTC has been synced with GNSS
volatile bool alarmFlag           = true;     // RTC alarm true when interrupt (initialized as true for first loop)
volatile bool initSetup           = true;     // False once GNSS messages configured-will not configure again
unsigned long prevMillis          = 0;        // Global time keeper, not affected by Millis rollover
unsigned long dates[16]           = {};       // Array with Unix Epochs of log dates !!! MAX 15 !!!
int           settings[16]        = {};       // Array that holds user settings on SD
char          line[100];                      // Temporary array for parsing user settings
char          logFileNameDate[30] = "";       // Log file name for modes 1, 2, 3


const char* toWrite = "In meteorology, a cloud is an aerosol consisting of a visible mass of miniature liquid droplets, frozen crystals, or other particles suspended in the atmosphere of a planetary body or similar space.[1] Water or various other chemicals may compose the droplets and crystals. On Earth, clouds are formed as a result of saturation of the air when it is cooled to its dew point, or when it gains sufficient moisture (usually in the form of water vapor) from an adjacent source to raise the dew point to the ambient temperature. They are seen in the Earth's homosphere, which includes the troposphere, stratosphere, and mesosphere. Nephology is the science of clouds, which is undertaken in the cloud physics branch of meteorology. There are two methods of naming clouds in their respective layers of the homosphere, Latin and common name. Genus types in the troposphere, the atmospheric layer closest to Earth's surface, have Latin names because of the universal adoption of Luke Howard's nomenclature that was formally proposed in 1802. It became the basis of a modern international system that divides clouds into five physical forms which can be further divided or classified into altitude levels to derive ten basic genera. The main representative cloud types for each of these forms are stratiform, cumuliform, stratocumuliform, cumulonimbiform, and cirriform. Low-level clouds do not have any altitude-related prefixes. However mid-level stratiform and stratocumuliform types are given the prefix alto- while high-level variants of these same two forms carry the prefix cirro-. In both cases, strato- is dropped from the latter form to avoid double-prefixing. Genus types with sufficient vertical extent to occupy more than one level do not carry any altitude related prefixes. They are classified formally as low- or mid-level depending on the altitude at which each initially forms, and are also more informally characterized as multi-level or vertical. Most of the ten genera derived by this method of classification can be subdivided into species and further subdivided into varieties. Very low stratiform clouds that extend down to the Earth's surface are given the common names fog and mist, but have no Latin names.";

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("***WELCOME TO GNSS LOGGER v1.0.6 (7/06/22)***");
  pinMode(LED, OUTPUT);              //
  pinMode(PER_POWER, OUTPUT);
  digitalWrite(PER_POWER, HIGH);
  mySpi.begin();
  delay(1);
  configureSD();
  if (!debugFile.open("test.csv", O_CREAT | O_APPEND | O_WRITE)) { // Create file if new, Append to end, Open to Write
    Serial.println("Warning: Failed to create debug file.");
    return;
  }
  
  else {
    Serial.println("Info: Created debug file"); 
  }

}

void loop() {
  digitalWrite(LED, HIGH);
  debugFile.println(toWrite);
  debugFile.println(toWrite);
  debugFile.println(toWrite);
  debugFile.println(toWrite);
  debugFile.println(toWrite);
  debugFile.println(toWrite);
  debugFile.println(toWrite);
  debugFile.println(toWrite);
  debugFile.println(toWrite);
  debugFile.println(toWrite);
  debugFile.println(toWrite);
  debugFile.println(toWrite);
  debugFile.println(toWrite);
  debugFile.println(toWrite);
  debugFile.println(toWrite);
  debugFile.println(toWrite);
  debugFile.println(toWrite);
  debugFile.println(toWrite);
  debugFile.println(toWrite);
  debugFile.println(toWrite);
  debugFile.println(toWrite);
  debugFile.println(toWrite);
  debugFile.println(toWrite);
  debugFile.println(toWrite);
  debugFile.println(toWrite);
  debugFile.println(toWrite);
  debugFile.println(toWrite);
  debugFile.println(toWrite);
  debugFile.println(toWrite);
  debugFile.println(toWrite);
  debugFile.println(toWrite);
  debugFile.println(toWrite);
  debugFile.println(toWrite);
  debugFile.println(toWrite);
  debugFile.println(toWrite);
  debugFile.println(toWrite);
  debugFile.println(toWrite);
  debugFile.println(toWrite);
  debugFile.println(toWrite);
  debugFile.println(toWrite);
  debugFile.println(toWrite);
  debugFile.println(toWrite);
  debugFile.println(toWrite);
  debugFile.println(toWrite);
  debugFile.println(toWrite);
  debugFile.println(toWrite);
  debugFile.println(toWrite);
  debugFile.println(toWrite);
  debugFile.println(toWrite);
  debugFile.println(toWrite);
  debugFile.println(toWrite);
  debugFile.println(toWrite);
  debugFile.println(toWrite);
  debugFile.println(toWrite);
  debugFile.println(toWrite);
  debugFile.println(toWrite);
  debugFile.println(toWrite);
  debugFile.println(toWrite);
  debugFile.println(toWrite);
  debugFile.println(toWrite);
  debugFile.println(toWrite);
  debugFile.println(toWrite);
  
  if (!debugFile.sync()) {
    Serial.println("Warning: Failed to sync debug file.");
  }
  
  digitalWrite(LED, LOW);
  delay(100);

}

void configureSD() {
  ///////// SD CARD INITIALIZATION
  if (!sd.begin(SD_CONFIG)) { // 
    Serial.println("Warning: Card failed, or not present. Trying again...");
    delay(2000);

    if (!sd.begin(SD_CONFIG)) { // 
      Serial.println("Warning: Card failed, or not present. Restarting...");
      while (1) {
        blinkLed(2, 250);
        delay(2000);
      }
    }
    else {
      Serial.println("Info: microSD initialized.");
    }
  }
  else {
    Serial.println("Info: microSD initialized.");
  }
}


void blinkLed(byte ledFlashes, unsigned int ledDelay) {
  byte i = 0;
  while (i < ledFlashes * 2) {
    unsigned long currMillis = millis();
    if (currMillis - prevMillis >= ledDelay) {
      digitalWrite(LED, !digitalRead(LED));
      prevMillis = currMillis;
      i++;
    }
  }
    // Turn off LED
  digitalWrite(LED, LOW);
}
