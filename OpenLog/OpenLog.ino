/*
  This is an example for OpenLog Artemis - based on a datalogging example from the SparkFun u-blox library:
  Configuring the GNSS to automatically send RXM SFRBX and RAWX reports over I2C and log them to file on SD card
  By: Paul Clark
  SparkFun Electronics
  Date: August 17th, 2021
  License: MIT. See license file for more information but you can
  basically do whatever you want with this code.
  This example shows how to configure the u-blox GNSS to send RXM SFRBX and RAWX reports automatically
  and log the data to SD card in UBX format
  This version uses v2.1.0 of the SparkFun Apollo3 (artemis) core.
  
  Please note: v2.1.1 of the core contains a change to the I2C interface which makes communication
  with u-blox modules over I2C less reliable. If you are building this code yourself,
  please use V2.1.0 of the core.
  
Fun Apollo3 boards installed: http://boardsmanager/All#SparkFun_Apollo3
  Press upload to upload the code onto the Artemis.
  Open the Serial Monitor at 115200 baud to see the output.
  Pull pin 32 low to stop logging. Logging will also stop on low battery.
  To minimise I2C bus errors, it is a good idea to open the I2C pull-up split pad links on
  the u-blox module breakout.
*/

#include <SPI.h>

#include <SparkFun_u-blox_GNSS_Arduino_Library.h> //Click here to get the library: http://librarymanager/All#SparkFun_u-blox_GNSS
SFE_UBLOX_GNSS myGNSS;

#include <SdFat.h> //SdFat v2.0.7 by Bill Greiman. Click here to get the library: http://librarymanager/All#SdFat_exFAT

#define SD_FAT_TYPE 3 // SD_FAT_TYPE = 0 for SdFat/File, 1 for FAT16/FAT32, 2 for exFAT, 3 for FAT16/FAT32 and exFAT.
#define SD_CONFIG SdSpiConfig(PIN_MICROSD_CHIP_SELECT, SHARED_SPI, SD_SCK_MHZ(24)) // 24MHz

#if SD_FAT_TYPE == 1
SdFat32 sd;
File32 myFile; //File that all GNSS data is written to
#elif SD_FAT_TYPE == 2
SdExFat sd;
ExFile myFile; //File that all GNSS data is written to
#elif SD_FAT_TYPE == 3
SdFs sd;
FsFile myFile; //File that all GNSS data is written to
#else // SD_FAT_TYPE == 0
SdFat sd;
File myFile; //File that all GNSS data is written to
#endif  // SD_FAT_TYPE

// OLA Specifics:

//Setup Qwiic Port
#include <Wire.h>
const byte PIN_QWIIC_SCL = 8;
const byte PIN_QWIIC_SDA = 9;
TwoWire qwiic(PIN_QWIIC_SDA,PIN_QWIIC_SCL); //Will use pads 8/9

//Define the pin functions
//FIX THESE.
const byte PIN_MICROSD_CHIP_SELECT = 23;
const byte PIN_IMU_POWER = 27;
const byte PIN_PWR_LED = 29;
const byte PIN_VREG_ENABLE = 25;
const byte PIN_VIN_MONITOR = 34; // VIN/3 (1M/2M - will require a correction factor)


const byte PIN_POWER_LOSS = 3;
const byte PIN_MICROSD_POWER = 15;
const byte PIN_QWIIC_POWER = 18;
const byte PIN_STAT_LED = 19;
const byte PIN_IMU_INT = 37;
const byte PIN_IMU_CHIP_SELECT = 44;
const byte PIN_STOP_LOGGING = 32;
const byte BREAKOUT_PIN_32 = 32;
const byte BREAKOUT_PIN_TX = 12;
const byte BREAKOUT_PIN_RX = 13;
const byte BREAKOUT_PIN_11 = 11;

// Globals and Consts

float vinCorrectionFactor = 1.47; //Correction factor for the VIN measurement; to compensate for the divider impedance
float lowBatteryThreshold = 3.4; // Low battery voltage threshold (Volts)
int lowBatteryReadings = 0; // Count how many times the battery voltage has read low
const int lowBatteryReadingsLimit = 10; // Don't declare the battery voltage low until we have had this many consecutive low readings (to reject sampling noise)
const int sdPowerDownDelay = 100; //Delay for this many ms before turning off the SD card power
bool powerLossSeen = false; //Interrupt flag for power loss detection
bool stopLoggingSeen = false; //Interrupt flag for stop logging detection
bool ignorePowerLossInterrupt = true; // Ignore the power loss interrupt - when attaching the interrupt
bool ignoreStopLoggingInterrupt = true; // Ignore the stop logging interrupt - when attaching the interrupt

// Data Logging Specifics:

#define sdWriteSize 512 // Write data to the SD card in blocks of 512 bytes
#define fileBufferSize 16384 // Allocate 16KBytes of RAM for UBX message storage

unsigned long lastPrint; // Record when the last Serial print took place
unsigned long bytesWritten = 0; // Record how many bytes have been written to SD card

// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

void setup()
{
  //If 3.3V rail drops below 3V, system will power down and maintain RTC
  pinMode(PIN_POWER_LOSS, INPUT); // BD49K30G-TL has CMOS output and does not need a pull-up

  delay(1); // Let PIN_POWER_LOSS stabilize

  if (digitalRead(PIN_POWER_LOSS) == LOW) powerLossISR(); //Check PIN_POWER_LOSS just in case we missed the falling edge
  ignorePowerLossInterrupt = true; // Ignore the power loss interrupt - when attaching the interrupt
  attachInterrupt(digitalPinToInterrupt(PIN_POWER_LOSS), powerLossISR, FALLING);
  ignorePowerLossInterrupt = false;

  powerLEDOn(); // Turn the power LED on - if the hardware supports it

  pinMode(PIN_STAT_LED, OUTPUT); // Flash the STAT LED during SD writes
  digitalWrite(PIN_STAT_LED, LOW);

  Serial.begin(115200);

  SPI.begin();

  //while (!Serial); //Wait for user to open terminal
  Serial.println("SparkFun OpenLog Artemis Example");
  Serial.println("u-blox GNSS RXM RAWX and SFRBS Logging");

  analogReadResolution(14); //Increase ADC resolution from default of 10-bit

  beginQwiic(); // Turn the qwiic power on as early as possible

  beginSD(); // Enable power for the SD card

  imuPowerOff(); // We're not using the IMU so turn it off

  delay(2500); // Give the GNSS time to power up

  pinMode(PIN_STOP_LOGGING, INPUT_PULLUP);
  delay(1); // Let the pin stabilize
  ignoreStopLoggingInterrupt = true; // Ignore the stop logging interrupt - when attaching the interrupt
  attachInterrupt(digitalPinToInterrupt(PIN_STOP_LOGGING), stopLoggingISR, FALLING); // Enable the stop logging interrupt
  pinMode(PIN_STOP_LOGGING, INPUT_PULLUP); //Re-attach the pull-up (bug in v2.1.0 of the core)
  ignoreStopLoggingInterrupt = false;

  Serial.println("Initializing SD card...");

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
  myFile.open("RXM_RAWX.ubx", FILE_WRITE);
  if(!myFile)
  {
    Serial.println(F("Failed to create UBX data file! Freezing..."));
    while (1);
  }

  //myGNSS.enableDebugging(); // Uncomment this line to enable lots of helpful GNSS debug messages on Serial
  //myGNSS.enableDebugging(Serial, true); // Or, uncomment this line to enable only the important GNSS debug messages on Serial

  myGNSS.disableUBX7Fcheck(); // RAWX data can legitimately contain 0x7F, so we need to disable the "7F" check in checkUbloxI2C

  // RAWX messages can be over 2KBytes in size, so we need to make sure we allocate enough RAM to hold all the data.
  // SD cards can occasionally 'hiccup' and a write takes much longer than usual. The buffer needs to be big enough
  // to hold the backlog of data if/when this happens.
  // getMaxFileBufferAvail will tell us the maximum number of bytes which the file buffer has contained.
  myGNSS.setFileBufferSize(fileBufferSize); // setFileBufferSize must be called _before_ .begin

  if (myGNSS.begin(qwiic) == false) //Connect to the u-blox module using Qwiic port
  {
    Serial.println(F("u-blox GNSS not detected at default I2C address. Please check wiring. Freezing..."));
    while (1);
  }

  // Uncomment the next line if you want to reset your module back to the default settings with 1Hz navigation rate
  // (This will also disable any "auto" messages that were enabled and saved by other examples and reduce the load on the I2C bus)
  //myGNSS.factoryDefault(); delay(5000);

  myGNSS.setI2COutput(COM_TYPE_UBX); //Set the I2C port to output UBX only (turn off NMEA noise)
  myGNSS.saveConfigSelective(VAL_CFG_SUBSEC_IOPORT); //Save (only) the communications port settings to flash and BBR
  myGNSS.setNavigationFrequency(1); //Produce one navigation solution per second (that's plenty for Precise Point Positioning)
  myGNSS.setAutoRXMSFRBX(true, false); // Enable automatic RXM SFRBX messages: without callback; without implicit update
  myGNSS.logRXMSFRBX(); // Enable RXM SFRBX data logging
  myGNSS.setAutoRXMRAWX(true, false); // Enable automatic RXM RAWX messages: without callback; without implicit update
  myGNSS.logRXMRAWX(); // Enable RXM RAWX data logging

  // If you want to log additional data, call the associated setAuto and log functions here

  while (Serial.available()) // Make sure the Serial buffer is empty
  {
    Serial.read();
  }

  Serial.println(F("Press any key to stop logging."));

  lastPrint = millis(); // Initialize lastPrint
}

// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

void loop()
{
  checkBattery(); // Check for low battery
  
  // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
  
  if ((powerLossSeen) || (stopLoggingSeen) || (Serial.available() > 0)) // Check for power loss or stop logging interrupts
  {
    stopLogging();
  }

  // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

  myGNSS.checkUblox(); // Check for the arrival of new data and process it.

  // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

  while (myGNSS.fileBufferAvailable() >= sdWriteSize) // Check to see if we have at least sdWriteSize waiting in the buffer
  {
    digitalWrite(PIN_STAT_LED, HIGH); // Flash PIN_STAT_LED each time we write to the SD card
  
    uint8_t myBuffer[sdWriteSize]; // Create our own buffer to hold the data while we write it to SD card

    myGNSS.extractFileBufferData((uint8_t *)&myBuffer, sdWriteSize); // Extract exactly sdWriteSize bytes from the UBX file buffer and put them into myBuffer

    myFile.write(myBuffer, sdWriteSize); // Write exactly sdWriteSize bytes from myBuffer to the ubxDataFile on the SD card

    bytesWritten += sdWriteSize; // Update bytesWritten

    // In case the SD writing is slow or there is a lot of data to write, keep checking for the arrival of new data
    myGNSS.checkUblox(); // Check for the arrival of new data and process it.

    digitalWrite(PIN_STAT_LED, LOW); // Turn PIN_STAT_LED off again
  }

  // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

  if (millis() > (lastPrint + 1000)) // Print bytesWritten once per second
  {
    Serial.print(F("The number of bytes written to SD card is ")); // Print how many bytes have been written to SD card
    Serial.println(bytesWritten);

    uint16_t maxBufferBytes = myGNSS.getMaxFileBufferAvail(); // Get how full the file buffer has been (not how full it is now)
    
    //Serial.print(F("The maximum number of bytes which the file buffer has contained is: ")); // It is a fun thing to watch how full the buffer gets
    //Serial.println(maxBufferBytes);

    if (maxBufferBytes > ((fileBufferSize / 5) * 4)) // Warn the user if fileBufferSize was more than 80% full
    {
      Serial.println(F("Warning: the file buffer has been over 80% full. Some data may have been lost."));
    }
    
    lastPrint = millis(); // Update lastPrint
  }
}

// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

//Stop Logging ISR
void stopLoggingISR(void){
  if (ignoreStopLoggingInterrupt == false)
  {
    Serial.println(F("Stop Logging Seen!"));
    detachInterrupt(digitalPinToInterrupt(PIN_STOP_LOGGING)); //Prevent multiple interrupts
    stopLoggingSeen = true;
  }
}

//Power Loss ISR
void powerLossISR(void){
  if (ignorePowerLossInterrupt == false)
  {
    Serial.println(F("Power Loss Detected!"));
    detachInterrupt(digitalPinToInterrupt(PIN_POWER_LOSS)); //Prevent multiple interrupts
    powerLossSeen = true;
  }
}

// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

void stopLogging(void){ // Stop logging; close the SD file; go into deep sleep
  Serial.println(F("stopLogging:"));
  
  uint16_t remainingBytes = myGNSS.fileBufferAvailable(); // Check if there are any bytes remaining in the file buffer
  
  while (remainingBytes > 0) // While there is still data in the file buffer
  {
    digitalWrite(PIN_STAT_LED, HIGH); // Flash PIN_STAT_LED while we write to the SD card
    
    uint8_t myBuffer[sdWriteSize]; // Create our own buffer to hold the data while we write it to SD card

    uint16_t bytesToWrite = remainingBytes; // Write the remaining bytes to SD card sdWriteSize bytes at a time
    if (bytesToWrite > sdWriteSize)
    {
      bytesToWrite = sdWriteSize;
    }

    myGNSS.extractFileBufferData((uint8_t *)&myBuffer, bytesToWrite); // Extract bytesToWrite bytes from the UBX file buffer and put them into myBuffer

    myFile.write(myBuffer, bytesToWrite); // Write bytesToWrite bytes from myBuffer to the ubxDataFile on the SD card

    bytesWritten += bytesToWrite; // Update bytesWritten

    remainingBytes -= bytesToWrite; // Decrement remainingBytes
  }

  digitalWrite(PIN_STAT_LED, LOW); // Turn PIN_STAT_LED off

  Serial.print(F("The total number of bytes written to SD card is ")); // Print how many bytes have been written to SD card
  Serial.println(bytesWritten);

  myFile.close(); // Close the data file
  
  Serial.println(F("Logging stopped. Freezing..."));
  Serial.flush(); // Make sure final message is printed

  delay(sdPowerDownDelay); // Give the SD card time to finish writing ***** THIS IS CRITICAL *****

  detachInterrupt(digitalPinToInterrupt(PIN_STOP_LOGGING)); //Prevent stop logging button from waking us from sleep
  detachInterrupt(digitalPinToInterrupt(PIN_POWER_LOSS)); //Prevent voltage supervisor from waking us from sleep

  qwiic.end(); //Power down I2C
  SPI.end(); //Power down SPI
  powerControlADC(false); //Power down ADC. It it started by default before setup().
  Serial.end(); //Power down UART
  
  powerLEDOff(); // Turn the power LED off - if the hardware supports it
  imuPowerOff();
  microSDPowerOff();
  qwiicPowerOff(); // Power off the Qwiic bus

//Force the peripherals off
  am_hal_pwrctrl_periph_disable(AM_HAL_PWRCTRL_PERIPH_IOM0);
  am_hal_pwrctrl_periph_disable(AM_HAL_PWRCTRL_PERIPH_IOM1);
  am_hal_pwrctrl_periph_disable(AM_HAL_PWRCTRL_PERIPH_IOM2);
  am_hal_pwrctrl_periph_disable(AM_HAL_PWRCTRL_PERIPH_IOM3);
  am_hal_pwrctrl_periph_disable(AM_HAL_PWRCTRL_PERIPH_IOM4);
  am_hal_pwrctrl_periph_disable(AM_HAL_PWRCTRL_PERIPH_IOM5);
  am_hal_pwrctrl_periph_disable(AM_HAL_PWRCTRL_PERIPH_ADC);
  am_hal_pwrctrl_periph_disable(AM_HAL_PWRCTRL_PERIPH_UART0);
  am_hal_pwrctrl_periph_disable(AM_HAL_PWRCTRL_PERIPH_UART1);

  //Disable pads
  for (int x = 0; x < 50; x++)
  {
    if ((x != PIN_MICROSD_POWER) &&
        //(x != PIN_LOGIC_DEBUG) &&
        (x != PIN_QWIIC_POWER) &&
        (x != PIN_IMU_POWER))
    {
      am_hal_gpio_pinconfig(x, g_AM_HAL_GPIO_DISABLE);
    }
  }

  //We can't leave these power control pins floating
  imuPowerOff();
  microSDPowerOff();
  qwiicPowerOff(); // Power off the Qwiic bus

  //Power down cache, flash, SRAM
  am_hal_pwrctrl_memory_deepsleep_powerdown(AM_HAL_PWRCTRL_MEM_ALL); // Power down all flash and cache
  am_hal_pwrctrl_memory_deepsleep_retain(AM_HAL_PWRCTRL_MEM_SRAM_384K); // Retain all SRAM

  //Keep the 32kHz clock running for RTC
  am_hal_stimer_config(AM_HAL_STIMER_CFG_CLEAR | AM_HAL_STIMER_CFG_FREEZE);
  am_hal_stimer_config(AM_HAL_STIMER_XTAL_32KHZ);

  while (1) // Stay in deep sleep until we get reset
  {
    am_hal_sysctrl_sleep(AM_HAL_SYSCTRL_SLEEP_DEEP); //Sleep
  }
}

// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

void powerLEDOn(){
  pinMode(PIN_PWR_LED, OUTPUT);
  digitalWrite(PIN_PWR_LED, HIGH); // Turn the Power LED on
}
void powerLEDOff(){
  pinMode(PIN_PWR_LED, OUTPUT);
  digitalWrite(PIN_PWR_LED, LOW); // Turn the Power LED off
}
void qwiicPowerOn(){
  pinMode(PIN_QWIIC_POWER, OUTPUT);
  digitalWrite(PIN_QWIIC_POWER, HIGH);
}
void qwiicPowerOff(){
  pinMode(PIN_QWIIC_POWER, OUTPUT);
  digitalWrite(PIN_QWIIC_POWER, LOW);
}
void microSDPowerOn(){
  pinMode(PIN_MICROSD_POWER, OUTPUT);
  digitalWrite(PIN_MICROSD_POWER, LOW);
}
void microSDPowerOff(){
  pinMode(PIN_MICROSD_POWER, OUTPUT);
  digitalWrite(PIN_MICROSD_POWER, HIGH);
}
void imuPowerOn(){
  pinMode(PIN_IMU_POWER, OUTPUT);
  digitalWrite(PIN_IMU_POWER, HIGH);
}
void imuPowerOff(){
  pinMode(PIN_IMU_POWER, OUTPUT);
  digitalWrite(PIN_IMU_POWER, LOW);
}
void beginQwiic(){
  pinMode(PIN_QWIIC_POWER, OUTPUT);
  qwiicPowerOn();
  qwiic.begin();
  setQwiicPullups(0); //Just to make it really clear what pull-ups are being used, set pullups here.
}
void setQwiicPullups(uint32_t i2cBusPullUps){
  //Change SCL and SDA pull-ups manually using pin_config
  am_hal_gpio_pincfg_t sclPinCfg = g_AM_BSP_GPIO_IOM1_SCL;
  am_hal_gpio_pincfg_t sdaPinCfg = g_AM_BSP_GPIO_IOM1_SDA;

  if (i2cBusPullUps == 0)
  {
    sclPinCfg.ePullup = AM_HAL_GPIO_PIN_PULLUP_NONE; // No pull-ups
    sdaPinCfg.ePullup = AM_HAL_GPIO_PIN_PULLUP_NONE;
  }
  else if (i2cBusPullUps == 1)
  {
    sclPinCfg.ePullup = AM_HAL_GPIO_PIN_PULLUP_1_5K; // Use 1K5 pull-ups
    sdaPinCfg.ePullup = AM_HAL_GPIO_PIN_PULLUP_1_5K;
  }
  else if (i2cBusPullUps == 6)
  {
    sclPinCfg.ePullup = AM_HAL_GPIO_PIN_PULLUP_6K; // Use 6K pull-ups
    sdaPinCfg.ePullup = AM_HAL_GPIO_PIN_PULLUP_6K;
  }
  else if (i2cBusPullUps == 12)
  {
    sclPinCfg.ePullup = AM_HAL_GPIO_PIN_PULLUP_12K; // Use 12K pull-ups
    sdaPinCfg.ePullup = AM_HAL_GPIO_PIN_PULLUP_12K;
  }
  else
  {
    sclPinCfg.ePullup = AM_HAL_GPIO_PIN_PULLUP_24K; // Use 24K pull-ups
    sdaPinCfg.ePullup = AM_HAL_GPIO_PIN_PULLUP_24K;
  }

  pin_config(PinName(PIN_QWIIC_SCL), sclPinCfg);
  pin_config(PinName(PIN_QWIIC_SDA), sdaPinCfg);
}
void beginSD(){
  pinMode(PIN_MICROSD_POWER, OUTPUT);
  pinMode(PIN_MICROSD_CHIP_SELECT, OUTPUT);
  digitalWrite(PIN_MICROSD_CHIP_SELECT, HIGH); //Be sure SD is deselected

  // For reasons I don't understand, we seem to have to wait for at least 1ms after SPI.begin before we call microSDPowerOn.
  // If you comment the next line, the Artemis resets at microSDPowerOn when beginSD is called from wakeFromSleep...
  // But only on one of my V10 red boards. The second one I have doesn't seem to need the delay!?
  delay(1);

  microSDPowerOn();
}

// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

//Read the VIN voltage
float readVIN(){
  // Only supported on >= V10 hardware
  int div3 = analogRead(PIN_VIN_MONITOR); //Read VIN across a 1/3 resistor divider
  float vin = (float)div3 * 3.0 * 2.0 / 16384.0; //Convert 1/3 VIN to VIN (14-bit resolution)
  vin = vin * vinCorrectionFactor; //Correct for divider impedance (determined experimentally)
  //Serial.print(F("VIN (Volts): "));
  //Serial.println(vin, 2);
  return (vin);
}

// Read the battery voltage
// If it is low, increment lowBatteryReadings
// If lowBatteryReadings exceeds lowBatteryReadingsLimit then powerDown
void checkBattery(void){
  float voltage = readVIN(); // Read the battery voltage
  if (voltage < lowBatteryThreshold) // Is the voltage low?
  {
    lowBatteryReadings++; // Increment the low battery count
    if (lowBatteryReadings > lowBatteryReadingsLimit) // Have we exceeded the low battery count limit?
    {
      // Gracefully powerDown
      Serial.println(F("Low battery detected!"));
      stopLogging();
    }
  }
  else
  {
    lowBatteryReadings = 0; // Reset the low battery count
  }
}
