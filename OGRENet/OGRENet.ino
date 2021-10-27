/*
   OGRENet: On-ice GNSS Research Experimental Network for Greenland
   By: Derek Pickell
   V0.0.1 (pre-release)

   Hardware:
   - Artemis MicroMod v.
   - Ublox ZED-F9P-01B-01

   Dependencies:
   - SparkFun_u-blox_GNSS_Arduino_Library v2.0.17
   - Apollo3 Arduino Core v2.1.0
*/

///////// LIBRARIES & OBJECT INSTANTIATION(S)
#include <Wire.h>
#include <SD.h>
#include <SPI.h>
#include <SparkFun_u-blox_GNSS_Arduino_Library.h> // Library v2.1.1: http://librarymanager/All#SparkFun_u-blox_GNSS
SFE_UBLOX_GNSS myGNSS;
//////////////////

///////// SD CARD & BUFFER SIZES
File myFile; //File that all GNSS data is written to
#define sdWriteSize 512 // Write data to the SD card in blocks of 512 bytes
#define fileBufferSize 16384 // Allocate 16KBytes of RAM for UBX message storage
//////////////////

///////// PINOUTS
const int ledPin = LED_BUILTIN;
const byte sdChipSelect = 41; //Primary SPI Chip Select is CS for the MicroMod Artemis Processor
#define PIN_QWIIC_POWER   34  // G2 - Drive low to turn off Peripheral
#define PIN_SD_CS         41  // CS
//////////////////

///////// TIMING - SPECIFY INTERVAL HERE -  -------------------------
uint32_t msToSleep = 10000; //SLEEP INTERVAL (MS)
const long interval = 10000; //LOGGING INTERVAL (MS)
---------------------------------------------------------------------
unsigned long PREVIOUS_MILLIS = 0;
#define TIMER_FREQ 32768L //CTimer6 will use the 32kHz clock
uint32_t sysTicksToSleep = msToSleep * TIMER_FREQ / 1000;
//////////////////

///////// COUNTING
//int count = 0; //counter
unsigned long bytesWritten = 0;
unsigned long lastPrint = 0; //used to keep time for logging
//////////////////

///////// DEBUGGING
#define DEBUG true // Output messages to Serial monitor
//////////////////

void setup(void) {
  Serial.begin(115200);
  Serial.println("OGRENet GNSS LOGGER");
  Wire.begin();
  SPI.begin();
  pinMode(ledPin, OUTPUT);

  ///////// On Apollo3 v2 MANUALLY DISABLE PULLUPS
  // The IOM and pin numbers here are specific to the Artemis MicroMod Processor Board
  am_hal_gpio_pincfg_t sclPinCfg = g_AM_BSP_GPIO_IOM4_SCL;  // Artemis MicroMod Processor Board uses IOM4 for I2C communication
  am_hal_gpio_pincfg_t sdaPinCfg = g_AM_BSP_GPIO_IOM4_SDA;
  sclPinCfg.ePullup = AM_HAL_GPIO_PIN_PULLUP_NONE;          // Disable the SCL/SDA pull-ups
  sdaPinCfg.ePullup = AM_HAL_GPIO_PIN_PULLUP_NONE;
  pin_config(PinName(39), sclPinCfg);                       // Artemis MicroMod Processor Board uses Pin/Pad 39 for SCL
  pin_config(PinName(40), sdaPinCfg);                       // Artemis MicroMod Processor Board uses Pin/Pad 40 for SDA

  ///////// SD CARD INITIALIZATION
  while (Serial.available()) {
    Serial.read();
  }

  Serial.println("Initializing SD card...");
  if (!SD.begin(sdChipSelect)) {
    Serial.println("Card failed, or not present. Freezing...");
    // don't do anything more:
    while (1);
  }
  Serial.println("SD card initialized.");

  // Create or open a file called "RXM_RAWX.ubx" on the SD card.
  // If the file already exists, the new data is appended to the end of the file.
  // If using longer logging timescales, consider creating new file each time?
  myFile = SD.open("RAWX.txt", FILE_WRITE);
  if (!myFile)
  {
    Serial.println(F("Failed to create UBX data file! Freezing..."));
    while (1);
  }

  ///////// GNSS SETTINGS
  configureGNSS();
  /////////

}

void loop(void) {
  digitalWrite(ledPin, HIGH);

  if (millis() - PREVIOUS_MILLIS > interval) {
    ///////// FINISH LOGGING
    Serial.println(millis());
    uint16_t remainingBytes = myGNSS.fileBufferAvailable(); // Check if there are any bytes remaining in the file buffer
    while (remainingBytes > 0) { // While there is still data in the file buffer
      uint8_t myBuffer[sdWriteSize]; // Create our own buffer to hold the data while we write it to SD card
      uint16_t bytesToWrite = remainingBytes; // Write the remaining bytes to SD card sdWriteSize bytes at a time
      if (bytesToWrite > sdWriteSize) {
        bytesToWrite = sdWriteSize;
      }
      myGNSS.extractFileBufferData((uint8_t *)&myBuffer, bytesToWrite); // Extract bytesToWrite bytes from the UBX file buffer and put them into myBuffer
      myFile.write(myBuffer, bytesToWrite); // Write bytesToWrite bytes from myBuffer to the ubxDataFile on the SD card
      remainingBytes -= bytesToWrite; // Decrement remainingBytes
    }

    ///////// SLEEP SHUTDOWN PREP
    Serial.println("timeout, going to sleep");
    delay(100);
    myFile.close();
    bytesWritten = 0;
    lastPrint = 0;
    Serial.flush();
    delay(100);
    goToSleep();
    /////////
  }

  ///////// DO GNSS STUFF
  myGNSS.checkUblox(); // Check for the arrival of new data and process it.
  while (myGNSS.fileBufferAvailable() >= sdWriteSize) { // Check to see if we have at least sdWriteSize waiting in the buffer
    uint8_t myBuffer[sdWriteSize]; // Create our own buffer to hold the data while we write it to SD card
    myGNSS.extractFileBufferData((uint8_t *)&myBuffer, sdWriteSize); // Extract exactly sdWriteSize bytes from the UBX file buffer and put them into myBuffer
    myFile.write(myBuffer, sdWriteSize); // Write exactly sdWriteSize bytes from myBuffer to the ubxDataFile on the SD card
    bytesWritten += sdWriteSize; // Update bytesWritten
    myGNSS.checkUblox(); // Check for the arrival of new data and process it if SD slow
  }

  ///////// PRINT BYTES WRITTEN
  if (millis() > (lastPrint + 1000)) { // Print bytesWritten once per second
    Serial.print(F("The number of bytes written to SD card is ")); // Print how many bytes have been written to SD card
    Serial.println(bytesWritten);
    uint16_t maxBufferBytes = myGNSS.getMaxFileBufferAvail(); // Get how full the file buffer has been (not how full it is now)
    if (maxBufferBytes > ((fileBufferSize / 5) * 4)) { // Warn the user if fileBufferSize was more than 80% full
      Serial.println(F("Warning: the file buffer has been over 80% full. Some data may have been lost."));
    }
    lastPrint = millis(); // Update lastPrint
  }
}


// CALLED ONCE NUMBER OF MS HAVE PASSED
extern "C" void am_stimer_cmpr6_isr(void)
{
  uint32_t ui32Status = am_hal_stimer_int_status_get(false);
  if (ui32Status & AM_HAL_STIMER_INT_COMPAREG)
  {
    am_hal_stimer_int_clear(AM_HAL_STIMER_INT_COMPAREG);
  }
}
