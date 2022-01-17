/*
RAWX LOGGING SIMPLE
*/

#include <SPI.h>
#include <Wire.h> //Needed for I2C to GNSS
#include <SdFat.h>

#include <SparkFun_u-blox_GNSS_Arduino_Library.h> //Click here to get the library: http://librarymanager/All#SparkFun_u-blox_GNSS

SFE_UBLOX_GNSS myGNSS;
SdFs sd;
File myFile; //File that all GNSS data is written to


///////// PINOUTS
#define LED                   16
#define ZED_POWER             34  
#define PERIPHERAL_POWER      18
#define PIN_SD_CS             41  
//#define SDA                   25
//#define SCL                   27
#define COPI                  38
#define CIPO                  43
#define SCK                   42
#define ZED_CTRL              34
#define PER_CTRL              18

//////////////////


/////// i2c SETUP
TwoWire myWire(2); // SDA 25 SCL 27

//////// spi
SPIClass mySPI(3); //Use IO Master 4 on pads 39/40/38
#define SD_CONFIG             SdSpiConfig(PIN_SD_CS, DEDICATED_SPI, SD_SCK_MHZ(24), &mySPI)

#define DEBUG true
#if DEBUG
#define DEBUG_PRINTLN(x)  Serial.println(x)
#define DEBUG_SERIALFLUSH() Serial.flush()
#else
#define DEBUG_PRINTLN(x)
#define DEBUG_SERIALFLUSH()
#endif

#define sdWriteSize 512 // Write data to the SD card in blocks of 512 bytes
#define fileBufferSize 16384 // Allocate 16KBytes of RAM for UBX message storage

unsigned long lastPrint; // Record when the last Serial print took place

// Note: we'll keep a count of how many SFRBX and RAWX messages arrive - but the count will not be completely accurate.
// If two or more SFRBX messages arrive together as a group and are processed by one call to checkUblox, the count will
// only increase by one.

int numSFRBX = 0; // Keep count of how many SFRBX message groups have been received (see note above)
int numRAWX = 0; // Keep count of how many RAWX message groups have been received (see note above)

// Callback: newSFRBX will be called when new RXM SFRBX data arrives
// See u-blox_structs.h for the full definition of UBX_RXMSFRBX_data_t
//         _____  You can use any name you like for the callback. Use the same name when you call setAutoRXMSFRBXcallback
//        /                  _____  This _must_ be UBX_RXM_SFRBX_data_t
//        |                 /                   _____ You can use any name you like for the struct
//        |                 |                  /
//        |                 |                  |
void newSFRBX(UBX_RXM_SFRBX_data_t ubxDataStruct)
{
  numSFRBX++; // Increment the count
}

// Callback: newRAWX will be called when new RXM RAWX data arrives
// See u-blox_structs.h for the full definition of UBX_RXMRAWX_data_t
//         _____  You can use any name you like for the callback. Use the same name when you call setAutoRXMRAWXcallback
//        /             _____  This _must_ be UBX_RXM_RAWX_data_t
//        |            /                _____ You can use any name you like for the struct
//        |            |               /
//        |            |               |
void newRAWX(UBX_RXM_RAWX_data_t ubxDataStruct)
{
  numRAWX++; // Increment the count
}

void setup()

{
  #if DEBUG
    Serial.begin(115200);
    while (!Serial); //Wait for user to open terminal
    Serial.println("SparkFun u-blox Example");
  #endif
  pinMode(LED_BUILTIN, OUTPUT); // Flash LED_BUILTIN each time we write to the SD card
  digitalWrite(LED_BUILTIN, LOW);

  pinMode(ZED_CTRL, OUTPUT);
  digitalWrite(ZED_CTRL, LOW);
  pinMode(PER_CTRL, OUTPUT);
  digitalWrite(PER_CTRL, LOW);
  

  myWire.begin(); // Start I2C communication
  myWire.setPullups(0);
  myWire.setClock(400000);

// On the Artemis, we can disable the internal I2C pull-ups too to help reduce bus errors

  #if DEBUG
    while (Serial.available()) // Make sure the Serial buffer is empty
    {
      Serial.read();
    }
  
    Serial.println(F("Press any key to start logging."));
  
    while (!Serial.available()) // Wait for the user to press a key
    {
      ; // Do nothing
    }
  
    delay(100); // Wait, just in case multiple characters were sent
  
    while (Serial.available()) // Empty the Serial buffer
    {
      Serial.read();
    }
  
    Serial.println("Initializing SD card...");
  #endif

  mySPI.begin();
  
  // See if the card is present and can be initialized:
  if (!sd.begin(SD_CONFIG))
  {
    Serial.println("Card failed, or not present. Freezing...");
    // don't do anything more:
    while (1);
  }
  Serial.println("SD card initialized.");

  // Create or open a file called "RXM_RAWX.ubx" on the SD card.
  // If the file already exists, the new data is appended to the end of the file.
  myFile = sd.open("BB.ubx", FILE_WRITE);
  if(!myFile)
  {
    #if DEBUG
      Serial.println(F("Failed to create UBX data file! Freezing..."));
      while (1);
    #endif
  }

 // myGNSS.enableDebugging(); // Uncomment this line to enable lots of helpful GNSS debug messages on Serial
  myGNSS.enableDebugging(Serial, true); // Or, uncomment this line to enable only the important GNSS debug messages on Serial

  myGNSS.disableUBX7Fcheck(); // RAWX data can legitimately contain 0x7F, so we need to disable the "7F" check in checkUbloxI2C

  // RAWX messages can be over 2KBytes in size, so we need to make sure we allocate enough RAM to hold all the data.
  // SD cards can occasionally 'hiccup' and a write takes much longer than usual. The buffer needs to be big enough
  // to hold the backlog of data if/when this happens.
  // getMaxFileBufferAvail will tell us the maximum number of bytes which the file buffer has contained.
  myGNSS.setFileBufferSize(fileBufferSize); // setFileBufferSize must be called _before_ .begin

  if (myGNSS.begin(myWire) == false) //Connect to the u-blox module using Wire port
  {
    #if DEBUG
      Serial.println(F("u-blox GNSS not detected at default I2C address. Please check wiring. Freezing..."));
      while (1);
    #endif
  }

  // Uncomment the next line if you want to reset your module back to the default settings with 1Hz navigation rate
  // (This will also disable any "auto" messages that were enabled and saved by other examples and reduce the load on the I2C bus)
  //myGNSS.factoryDefault(); delay(5000);

  myGNSS.newCfgValset8(UBLOX_CFG_SIGNAL_GPS_ENA, 1);   // Enable/Disable GPS
  myGNSS.addCfgValset8(UBLOX_CFG_SIGNAL_GLO_ENA, 1);   // Disable GLONASS
  myGNSS.addCfgValset8(UBLOX_CFG_SIGNAL_GAL_ENA, 0);   // Disable Galileo
  myGNSS.addCfgValset8(UBLOX_CFG_SIGNAL_BDS_ENA, 0);   // Disable BeiDou
  myGNSS.sendCfgValset8(UBLOX_CFG_SIGNAL_QZSS_ENA, 0); // Disable QZSS
  

  myGNSS.setI2COutput(COM_TYPE_UBX); //Set the I2C port to output UBX only (turn off NMEA noise)
  myGNSS.saveConfigSelective(VAL_CFG_SUBSEC_IOPORT); //Save (only) the communications port settings to flash and BBR
  myGNSS.setNavigationFrequency(1); //Produce one navigation solution per second (that's plenty for Precise Point Positioning)
  myGNSS.setAutoRXMSFRBXcallback(&newSFRBX); // Enable automatic RXM SFRBX messages with callback to newSFRBX
  myGNSS.logRXMSFRBX(); // Enable RXM SFRBX data logging
  myGNSS.setAutoRXMRAWXcallback(&newRAWX); // Enable automatic RXM RAWX messages with callback to newRAWX
  myGNSS.logRXMRAWX(); // Enable RXM RAWX data logging

  #if DEBUG
    Serial.println(F("Press any key to stop logging."));
  #endif
  lastPrint = millis(); // Initialize lastPrint
}

void loop()
{
  // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

  myGNSS.checkUblox(); // Check for the arrival of new data and process it.
  myGNSS.checkCallbacks(); // Check if any callbacks are waiting to be processed.

  // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

  while (myGNSS.fileBufferAvailable() >= sdWriteSize) // Check to see if we have at least sdWriteSize waiting in the buffer
  {
    digitalWrite(LED_BUILTIN, HIGH); // Flash LED_BUILTIN each time we write to the SD card

    uint8_t myBuffer[sdWriteSize]; // Create our own buffer to hold the data while we write it to SD card

    myGNSS.extractFileBufferData((uint8_t *)&myBuffer, sdWriteSize); // Extract exactly sdWriteSize bytes from the UBX file buffer and put them into myBuffer

    myFile.write(myBuffer, sdWriteSize); // Write exactly sdWriteSize bytes from myBuffer to the ubxDataFile on the SD card
    // In case the SD writing is slow or there is a lot of data to write, keep checking for the arrival of new data
    myGNSS.checkUblox(); // Check for the arrival of new data and process it.
    myGNSS.checkCallbacks(); // Check if any callbacks are waiting to be processed.

    digitalWrite(LED_BUILTIN, LOW); // Turn LED_BUILTIN off again
  }
  myFile.flush();
  // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
  #if DEBUG
    if (millis() > (lastPrint + 1000)) // Print the message count once per second
    {
      Serial.print(F("Number of message groups received: SFRBX: ")); // Print how many message groups have been received (see note above)
      Serial.print(numSFRBX);
      Serial.print(F(" RAWX: "));
      Serial.println(numRAWX);
  
      uint16_t maxBufferBytes = myGNSS.getMaxFileBufferAvail(); // Get how full the file buffer has been (not how full it is now)
  
      //Serial.print(F("The maximum number of bytes which the file buffer has contained is: ")); // It is a fun thing to watch how full the buffer gets
      //Serial.println(maxBufferBytes);
  
      if (maxBufferBytes > ((fileBufferSize / 5) * 4)) // Warn the user if fileBufferSize was more than 80% full
      {
        Serial.println(F("Warning: the file buffer has been over 80% full. Some data may have been lost."));
      }
  
      lastPrint = millis(); // Update lastPrint
    }
  #endif

  // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
//  uint16_t remainingBytes = myGNSS.fileBufferAvailable(); // Check if there are any bytes remaining in the file buffer
//  
//  while (remainingBytes > 0) // While there is still data in the file buffer
//  {
//        digitalWrite(LED_BUILTIN, HIGH); // Flash LED_BUILTIN while we write to the SD card
//  
//        uint8_t myBuffer[sdWriteSize]; // Create our own buffer to hold the data while we write it to SD card
//  
//        uint16_t bytesToWrite = remainingBytes; // Write the remaining bytes to SD card sdWriteSize bytes at a time
//        if (bytesToWrite > sdWriteSize)
//        {
//          bytesToWrite = sdWriteSize;
//        }
//  
//        myGNSS.extractFileBufferData((uint8_t *)&myBuffer, bytesToWrite); // Extract bytesToWrite bytes from the UBX file buffer and put them into myBuffer
//  
//        myFile.write(myBuffer, bytesToWrite); // Write bytesToWrite bytes from myBuffer to the ubxDataFile on the SD card
//  
//        remainingBytes -= bytesToWrite; // Decrement remainingBytes
//      }
//  
//  digitalWrite(LED_BUILTIN, LOW); // Turn LED_BUILTIN off
//  
//  myFile.close(); // Close the data file
//  myFile = SD.open("RXM_RAWX.ubx", FILE_WRITE);

  #if DEBUG
    if (Serial.available()) // Check if the user wants to stop logging
    {
      uint16_t remainingBytes = myGNSS.fileBufferAvailable(); // Check if there are any bytes remaining in the file buffer
  
      while (remainingBytes > 0) // While there is still data in the file buffer
      {
        digitalWrite(LED_BUILTIN, HIGH); // Flash LED_BUILTIN while we write to the SD card
  
        uint8_t myBuffer[sdWriteSize]; // Create our own buffer to hold the data while we write it to SD card
  
        uint16_t bytesToWrite = remainingBytes; // Write the remaining bytes to SD card sdWriteSize bytes at a time
        if (bytesToWrite > sdWriteSize)
        {
          bytesToWrite = sdWriteSize;
        }
  
        myGNSS.extractFileBufferData((uint8_t *)&myBuffer, bytesToWrite); // Extract bytesToWrite bytes from the UBX file buffer and put them into myBuffer
  
        myFile.write(myBuffer, bytesToWrite); // Write bytesToWrite bytes from myBuffer to the ubxDataFile on the SD card
  
        remainingBytes -= bytesToWrite; // Decrement remainingBytes
      }
  
      digitalWrite(LED_BUILTIN, LOW); // Turn LED_BUILTIN off
  
      myFile.close(); // Close the data file
  
      Serial.println(F("Logging stopped. Freezing..."));
      while(1); // Do nothing more
    }
  #endif

  // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
}
