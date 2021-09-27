/*
*/

#include <Wire.h>
#include <SD.h>
#include <SPI.h>
#include <SparkFun_u-blox_GNSS_Arduino_Library.h> //Click here to get the library: http://librarymanager/All#SparkFun_u-blox_GNSS
SFE_UBLOX_GNSS myGNSS;

/////////SD STUFF
File myFile; //File that all GNSS data is written to
#define sdWriteSize 512 // Write data to the SD card in blocks of 512 bytes
#define fileBufferSize 16384 // Allocate 16KBytes of RAM for UBX message storage

/////////PINOUTS
const byte sdChipSelect = 41; //Primary SPI Chip Select is CS for the MicroMod Artemis Processor. Adjust for your processor if necessary.
const byte PIN_MICROSD_CHIP_SELECT = 55;
const byte PIN_MICROSD_POWER = 4;
const byte PIN_QWIIC_POWER = 44;
const byte PIN_QWIIC_SCL = 14;
const byte PIN_QWIIC_SCA = 12;

/////////TIMING
uint32_t msToSleep = 10000; //SLEEP INTERVAL (MS)
#define TIMER_FREQ 32768L //Counter/Timer 6 will use the 32kHz clock
uint32_t sysTicksToSleep = msToSleep * TIMER_FREQ / 1000;
unsigned long previousMillis = 0;
const long interval = 10000; //AWAKE INTERVAL
const int ledPin = LED_BUILTIN;

/////////COUNTING
//int count = 0; //counter
unsigned long bytesWritten = 0;
unsigned long lastPrint = 0; //used to keep time for logging

void setup(void) {
  Serial.begin(115200);
  Serial.println("GNSS SLEEPING LOGGER");
  Wire.begin();
  SPI.begin();

  pinMode(ledPin, OUTPUT);
  delay(2500);

  //SD CARD STUFF
   while (Serial.available()){
    Serial.read();
  }

  Serial.println("Initializing SD card...");
  if (!SD.begin(sdChipSelect)){
    Serial.println("Card failed, or not present. Freezing...");
    // don't do anything more:
    while (1);
  }
  Serial.println("SD card initialized.");

  // Create or open a file called "RXM_RAWX.ubx" on the SD card.
  // If the file already exists, the new data is appended to the end of the file.
  myFile = SD.open("RAWX.txt", FILE_WRITE);
  if(!myFile)
  {
    Serial.println(F("Failed to create UBX data file! Freezing..."));
    while (1);
  }

  /////////GNSS SETTINGS
  myGNSS.disableUBX7Fcheck(); // RAWX data can legitimately contain 0x7F, so we need to disable the "7F" check in checkUbloxI2C
  myGNSS.setFileBufferSize(fileBufferSize); // setFileBufferSize must be called _before_ .begin
  if (myGNSS.begin() == false){
    Serial.println(F("u-blox GNSS not detected at default I2C address. Please check wiring. Freezing..."));
    while (1);
  }
  
  myGNSS.setI2COutput(COM_TYPE_UBX); //Set the I2C port to output UBX only (turn off NMEA noise)
  myGNSS.saveConfigSelective(VAL_CFG_SUBSEC_IOPORT); //Save (only) the communications port settings to flash and BBR
  myGNSS.setNavigationFrequency(1); //Produce one navigation solution per second (that's plenty for Precise Point Positioning)
  myGNSS.setAutoRXMSFRBX(true, false); // Enable automatic RXM SFRBX messages 
  myGNSS.logRXMSFRBX(); // Enable RXM SFRBX data logging
  myGNSS.setAutoRXMRAWX(true, false); // Enable automatic RXM RAWX messages 
  myGNSS.logRXMRAWX(); // Enable RXM RAWX data logging
  
}

void loop(void) {
  digitalWrite(ledPin, HIGH);

  if (millis() > interval){
    //////////////FINISH LOGGING
    uint16_t remainingBytes = myGNSS.fileBufferAvailable(); // Check if there are any bytes remaining in the file buffer
    while (remainingBytes > 0){ // While there is still data in the file buffer
      uint8_t myBuffer[sdWriteSize]; // Create our own buffer to hold the data while we write it to SD card
      uint16_t bytesToWrite = remainingBytes; // Write the remaining bytes to SD card sdWriteSize bytes at a time
      if (bytesToWrite > sdWriteSize){
        bytesToWrite = sdWriteSize;
      }
      myGNSS.extractFileBufferData((uint8_t *)&myBuffer, bytesToWrite); // Extract bytesToWrite bytes from the UBX file buffer and put them into myBuffer
      myFile.write(myBuffer, bytesToWrite); // Write bytesToWrite bytes from myBuffer to the ubxDataFile on the SD card
      //bytesWritten += bytesToWrite; // Update bytesWritten
      remainingBytes -= bytesToWrite; // Decrement remainingBytes
    }    
    //////////////SLEEP SHUTDOWN PREP
    Serial.println("timeout, going to sleep");
    digitalWrite(ledPin, LOW);
    delay(1000);
    myFile.write("Logging Stopped");
    delay(10);
    myFile.close();
    bytesWritten = 0;
    lastPrint = 0;
    Serial.println("logging stopped");
    Serial.flush();
    delay(100);
    goToSleep();
    ////////////////
  }

  ////////////DO GNSS STUFF
  myGNSS.checkUblox(); // Check for the arrival of new data and process it.
  while (myGNSS.fileBufferAvailable() >= sdWriteSize){ // Check to see if we have at least sdWriteSize waiting in the buffer
      uint8_t myBuffer[sdWriteSize]; // Create our own buffer to hold the data while we write it to SD card
      myGNSS.extractFileBufferData((uint8_t *)&myBuffer, sdWriteSize); // Extract exactly sdWriteSize bytes from the UBX file buffer and put them into myBuffer
      myFile.write(myBuffer, sdWriteSize); // Write exactly sdWriteSize bytes from myBuffer to the ubxDataFile on the SD card
      bytesWritten += sdWriteSize; // Update bytesWritten
      myGNSS.checkUblox(); // Check for the arrival of new data and process it if SD slow
    }

  ////////////PRINT BYTES WRITTEN
  if (millis() > (lastPrint + 1000)){ // Print bytesWritten once per second
    Serial.print(F("The number of bytes written to SD card is ")); // Print how many bytes have been written to SD card
    Serial.println(bytesWritten);
    uint16_t maxBufferBytes = myGNSS.getMaxFileBufferAvail(); // Get how full the file buffer has been (not how full it is now)
    if (maxBufferBytes > ((fileBufferSize / 5) * 4)){ // Warn the user if fileBufferSize was more than 80% full
      Serial.println(F("Warning: the file buffer has been over 80% full. Some data may have been lost."));
    }
    lastPrint = millis(); // Update lastPrint
  }
}

//Power everything down and wait for interrupt wakeup
void goToSleep(){
  Wire.end(); //Power down I2C
  SPI.end(); //Power down SPI
  //qwiic.end(); //Power down QWIIC
  power_adc_disable(); //Power down ADC. It it started by default before setup().
  Serial.end(); //Power down UART
//  microSDPowerOff();
//  qwiicPowerOff();

//  //Force the peripherals off
//  am_hal_pwrctrl_periph_disable(AM_HAL_PWRCTRL_PERIPH_IOM0);
//  am_hal_pwrctrl_periph_disable(AM_HAL_PWRCTRL_PERIPH_IOM1);
//  am_hal_pwrctrl_periph_disable(AM_HAL_PWRCTRL_PERIPH_IOM2);
//  am_hal_pwrctrl_periph_disable(AM_HAL_PWRCTRL_PERIPH_IOM3);
//  am_hal_pwrctrl_periph_disable(AM_HAL_PWRCTRL_PERIPH_IOM4);
//  am_hal_pwrctrl_periph_disable(AM_HAL_PWRCTRL_PERIPH_IOM5);
//  am_hal_pwrctrl_periph_disable(AM_HAL_PWRCTRL_PERIPH_ADC);
//  am_hal_pwrctrl_periph_disable(AM_HAL_PWRCTRL_PERIPH_UART0);
//  am_hal_pwrctrl_periph_disable(AM_HAL_PWRCTRL_PERIPH_UART1);

  //Disable all pads
  for (int x = 0 ; x < 50 ; x++)
    am_hal_gpio_pinconfig(x , g_AM_HAL_GPIO_DISABLE);

//  microSDPowerOff();
//  qwiicPowerOff(); // Power off the Qwiic bus

  //We use counter/timer 6 to cause us to wake up from sleep but 0 to 7 are available
  //CT 7 is used for Software Serial. All CTs are used for Servo.
  am_hal_stimer_int_clear(AM_HAL_STIMER_INT_COMPAREG); //Clear CT6
  am_hal_stimer_int_enable(AM_HAL_STIMER_INT_COMPAREG); //Enable C/T G=6

  //Use the lower power 32kHz clock. Use it to run CT6 as well.
  am_hal_stimer_config(AM_HAL_STIMER_CFG_CLEAR | AM_HAL_STIMER_CFG_FREEZE);
  am_hal_stimer_config(AM_HAL_STIMER_XTAL_32KHZ | AM_HAL_STIMER_CFG_COMPARE_G_ENABLE);

  //Setup interrupt to trigger when the number of ms have elapsed AMOUNT OF TIME HERE *********
  am_hal_stimer_compare_delta_set(6, sysTicksToSleep); 

  //Power down Flash, SRAM, cache
  am_hal_pwrctrl_memory_deepsleep_powerdown(AM_HAL_PWRCTRL_MEM_CACHE); //Turn off CACHE
  am_hal_pwrctrl_memory_deepsleep_powerdown(AM_HAL_PWRCTRL_MEM_FLASH_512K); //Turn off everything but lower 512k
  am_hal_pwrctrl_memory_deepsleep_powerdown(AM_HAL_PWRCTRL_MEM_SRAM_64K_DTCM); //Turn off everything but lower 64k

  //Enable the timer interrupt in the NVIC.
  NVIC_EnableIRQ(STIMER_CMPR6_IRQn);

  //Go to Deep Sleep.
  am_hal_sysctrl_sleep(AM_HAL_SYSCTRL_SLEEP_DEEP);

  //Turn off interrupt
  NVIC_DisableIRQ(STIMER_CMPR6_IRQn);

  //We're BACK!
  wakeFromSleep();
}

//Power everything up gracefully
void wakeFromSleep(){

  //Power up SRAM, turn on entire Flash
  am_hal_pwrctrl_memory_deepsleep_powerdown(AM_HAL_PWRCTRL_MEM_MAX);
  
  //Go back to using the main clock
  //am_hal_stimer_int_enable(AM_HAL_STIMER_INT_OVERFLOW);
  //NVIC_EnableIRQ(STIMER_IRQn);
  am_hal_stimer_config(AM_HAL_STIMER_CFG_CLEAR | AM_HAL_STIMER_CFG_FREEZE);
  am_hal_stimer_config(AM_HAL_STIMER_HFRC_3MHZ);

  //Turn on ADC
  ap3_adc_setup();

//  pin_config(PinName(PIN_SPI_CIPO), g_AM_BSP_GPIO_IOM0_MISO); //SPI
//  pin_config(PinName(PIN_SPI_COPI), g_AM_BSP_GPIO_IOM0_MOSI);
//  pin_config(PinName(PIN_SPI_SCK), g_AM_BSP_GPIO_IOM0_SCK);

//  pin_config(PinName(PIN_QWIIC_SCL), g_AM_BSP_GPIO_IOM1_SCL);
//  pin_config(PinName(PIN_QWIIC_SDA), g_AM_BSP_GPIO_IOM1_SDA);

  //Set any pinModes
  pinMode(ledPin, OUTPUT);
  pinMode(ledPin, HIGH);

  //Turn on Serial
  Serial.begin(115200);
  delay(10);
  Serial.println("Back on, LED");

  //Turn on I2C
  Wire.begin();
  SPI.begin();
  //qwiic.begin();
  //beginSD();

  //CONFIM SD CARD RESTARTED
  if (!SD.begin(sdChipSelect)){
    Serial.println("Card failed, or not present. Freezing...");
    // don't do anything more:
    while (1);
  }
  myFile = SD.open("RAWX.txt", FILE_WRITE);
  delay(100);

  //CONFIRM QWIIC GNSS RESTARTED
  myGNSS.setFileBufferSize(fileBufferSize); // setFileBufferSize must be called _before_ .begin
  if (myGNSS.begin() == false){
    Serial.println(F("u-blox GNSS not detected at default I2C address. Please check wiring. Freezing..."));
    while (1);
  }
  Serial.println("restarted");
}

void beginSD(){
  pinMode(PIN_MICROSD_POWER, OUTPUT);
  pinMode(PIN_MICROSD_CHIP_SELECT, OUTPUT);
  digitalWrite(PIN_MICROSD_CHIP_SELECT, HIGH); //Be sure SD is deselected
  delay(1);

  microSDPowerOn();
}

void microSDPowerOn()
{
  pinMode(PIN_MICROSD_POWER, OUTPUT);
  digitalWrite(PIN_MICROSD_POWER, LOW);
}
void microSDPowerOff()
{
  pinMode(PIN_MICROSD_POWER, OUTPUT);
  digitalWrite(PIN_MICROSD_POWER, HIGH);
}

//void setQwiicPullups(uint32_t i2cBusPullUps)
//{
//  //Change SCL and SDA pull-ups manually using pin_config
//  am_hal_gpio_pincfg_t sclPinCfg = g_AM_BSP_GPIO_IOM1_SCL;
//  am_hal_gpio_pincfg_t sdaPinCfg = g_AM_BSP_GPIO_IOM1_SDA;
//
//  if (i2cBusPullUps == 0)
//  {
//    sclPinCfg.ePullup = AM_HAL_GPIO_PIN_PULLUP_NONE; // No pull-ups
//    sdaPinCfg.ePullup = AM_HAL_GPIO_PIN_PULLUP_NONE;
//  }
//  else if (i2cBusPullUps == 1)
//  {
//    sclPinCfg.ePullup = AM_HAL_GPIO_PIN_PULLUP_1_5K; // Use 1K5 pull-ups
//    sdaPinCfg.ePullup = AM_HAL_GPIO_PIN_PULLUP_1_5K;
//  }
//  else if (i2cBusPullUps == 6)
//  {
//    sclPinCfg.ePullup = AM_HAL_GPIO_PIN_PULLUP_6K; // Use 6K pull-ups
//    sdaPinCfg.ePullup = AM_HAL_GPIO_PIN_PULLUP_6K;
//  }
//  else if (i2cBusPullUps == 12)
//  {
//    sclPinCfg.ePullup = AM_HAL_GPIO_PIN_PULLUP_12K; // Use 12K pull-ups
//    sdaPinCfg.ePullup = AM_HAL_GPIO_PIN_PULLUP_12K;
//  }
//  else
//  {
//    sclPinCfg.ePullup = AM_HAL_GPIO_PIN_PULLUP_24K; // Use 24K pull-ups
//    sdaPinCfg.ePullup = AM_HAL_GPIO_PIN_PULLUP_24K;
//  }
//
//  pin_config(PinName(PIN_QWIIC_SCL), sclPinCfg);
//  pin_config(PinName(PIN_QWIIC_SDA), sdaPinCfg);
//}



//CALLED ONCE NUMBER OF MS HAVE PASSED
extern "C" void am_stimer_cmpr6_isr(void)
{
  uint32_t ui32Status = am_hal_stimer_int_status_get(false);
  if (ui32Status & AM_HAL_STIMER_INT_COMPAREG)
  {
    am_hal_stimer_int_clear(AM_HAL_STIMER_INT_COMPAREG);
  }
}
