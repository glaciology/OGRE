/*
*/

#include <Wire.h>
#include <SD.h>
#include <SPI.h>
#include <SparkFun_u-blox_GNSS_Arduino_Library.h> //Click here to get the library: http://librarymanager/All#SparkFun_u-blox_GNSS
SFE_UBLOX_GNSS myGNSS;

////////SPI STUFF
//#define SPI_SPEED 1000000
//#define SPI_ORDER MSBFIRST
//#define SPI_MODE SPI_MODE0
//SPISettings mySettings(SPI_SPEED, SPI_ORDER, SPI_MODE);

/////////SD STUFF
File myFile; //File that all GNSS data is written to
#define sdChipSelect 41 //Primary SPI Chip Select is CS for the MicroMod Artemis Processor. Adjust for your processor if necessary.
#define sdWriteSize 512 // Write data to the SD card in blocks of 512 bytes
#define fileBufferSize 16384 // Allocate 16KBytes of RAM for UBX message storage

/////////SLEEP TIME
uint32_t msToSleep = 30000; //This is the user editable number of ms to sleep between RTC checks
#define TIMER_FREQ 32768L //Counter/Timer 6 will use the 32kHz clock
uint32_t sysTicksToSleep = msToSleep * TIMER_FREQ / 1000;

/////////COLLECTING DATA
unsigned long previousMillis = 0;
const long interval = 100000;
const int ledPin = LED_BUILTIN;

/////////CALLBACKS
int numSFRBX = 0; // Keep count of how many SFRBX message groups have been received (see note above)
int numRAWX = 0; // Keep count of how many RAWX message groups have been received (see note above)

void newSFRBX(UBX_RXM_SFRBX_data_t ubxDataStruct){
  numSFRBX++; // Increment the count
}

void newRAWX(UBX_RXM_RAWX_data_t ubxDataStruct){
  numRAWX++; // Increment the count
}


void setup(void) {
  Serial.begin(115200);
  Serial.println("GNSS SLEEP LOG Example");
  Wire.begin();

  //We don't really use SPI in this example but we power it up and use it just to show its use.
//  SPI.begin();
//  SPI.beginTransaction(mySettings);
//  SPI.transfer(0xAA);
//  SPI.endTransaction();

  pinMode(ledPin, OUTPUT);

  //SD CARD STUFF
   while (Serial.available()){
    Serial.read();
  }

  Serial.println("Initializing SD card...");

  // See if the card is present and can be initialized:
  if (!SD.begin(sdChipSelect)){
    Serial.println("Card failed, or not present. Freezing...");
    // don't do anything more:
    while (1);
  }
  
  // INITIALIZATION
  Serial.println("SD card initialized.");

  // Create or open a file called "RXM_RAWX.ubx" on the SD card.
  // If the file already exists, the new data is appended to the end of the file.
  myFile = SD.open("RXM_RAWX.ubx", FILE_WRITE);
  if(!myFile)
  {
    Serial.println(F("Failed to create UBX data file! Freezing..."));
    while (1);
  }

  //myGNSS.enableDebugging(); // Uncomment this line to enable lots of helpful GNSS debug messages on Serial
  //myGNSS.enableDebugging(Serial, true); // Or, uncomment this line to enable only the important GNSS debug messages on Serial

  myGNSS.disableUBX7Fcheck(); // RAWX data can legitimately contain 0x7F, so we need to disable the "7F" check in checkUbloxI2C
  myGNSS.setFileBufferSize(fileBufferSize); // setFileBufferSize must be called _before_ .begin

  if (myGNSS.begin() == false){
    Serial.println(F("u-blox GNSS not detected at default I2C address. Please check wiring. Freezing..."));
    while (1);
  }

  // Uncomment the next line if you want to reset your module back to the default settings with 1Hz navigation rate
  // (This will also disable any "auto" messages that were enabled and saved by other examples and reduce the load on the I2C bus)
  //myGNSS.factoryDefault(); delay(5000);

  myGNSS.setI2COutput(COM_TYPE_UBX); //Set the I2C port to output UBX only (turn off NMEA noise)
  myGNSS.saveConfigSelective(VAL_CFG_SUBSEC_IOPORT); //Save (only) the communications port settings to flash and BBR
  myGNSS.setNavigationFrequency(1); //Produce one navigation solution per second (that's plenty for Precise Point Positioning)
  myGNSS.setAutoRXMSFRBXcallback(&newSFRBX); // Enable automatic RXM SFRBX messages with callback to newSFRBX
  myGNSS.logRXMSFRBX(); // Enable RXM SFRBX data logging
  myGNSS.setAutoRXMRAWXcallback(&newRAWX); // Enable automatic RXM RAWX messages with callback to newRAWX
  myGNSS.logRXMRAWX(); // Enable RXM RAWX data logging
}

void loop(void) {

  previousMillis = millis();
  Serial.println(previousMillis);
  
  while ((millis()-previousMillis) < interval){
    digitalWrite(ledPin, HIGH);

    //DO STUFF HERE
    myGNSS.checkUblox(); // Check for the arrival of new data and process it.
    myGNSS.checkCallbacks(); // Check if any callbacks are waiting to be processed.

    while (myGNSS.fileBufferAvailable() >= sdWriteSize) // Check to see if we have at least sdWriteSize waiting in the buffer
    {
      uint8_t myBuffer[sdWriteSize]; // Create our own buffer to hold the data while we write it to SD card
      myGNSS.extractFileBufferData((uint8_t *)&myBuffer, sdWriteSize); // Extract exactly sdWriteSize bytes from the UBX file buffer and put them into myBuffer
      myFile.write(myBuffer, sdWriteSize); // Write exactly sdWriteSize bytes from myBuffer to the ubxDataFile on the SD card
      // In case the SD writing is slow or there is a lot of data to write, keep checking for the arrival of new data
      myGNSS.checkUblox(); // Check for the arrival of new data and process it.
      myGNSS.checkCallbacks(); // Check if any callbacks are waiting to be processed.
    }

    //49 days later?
    if(millis()<previousMillis){
      previousMillis = millis();
    }
  }
  Serial.println("timeout, going to sleep");
  digitalWrite(ledPin, LOW);
  delay(1000);
  goToSleep();
}

//Power everything down and wait for interrupt wakeup
void goToSleep()
{
  Wire.end(); //Power down I2C
  SPI.end(); //Power down SPI
  //SPI1.end(); //This example doesn't use SPI1 but you will need to end any instance you may have created

  power_adc_disable(); //Power down ADC. It it started by default before setup().

  Serial.end(); //Power down UART
  //Serial1.end();

  //Disable all pads
  for (int x = 0 ; x < 50 ; x++)
    am_hal_gpio_pinconfig(x , g_AM_HAL_GPIO_DISABLE);

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
  //am_hal_pwrctrl_memory_deepsleep_powerdown(AM_HAL_PWRCTRL_MEM_ALL); //Turn off all memory (doesn't recover)

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
void wakeFromSleep()
{
  //Power up SRAM, turn on entire Flash
  am_hal_pwrctrl_memory_deepsleep_powerdown(AM_HAL_PWRCTRL_MEM_MAX);
  
  //Go back to using the main clock
  am_hal_stimer_int_enable(AM_HAL_STIMER_INT_OVERFLOW);
  NVIC_EnableIRQ(STIMER_IRQn);
  am_hal_stimer_config(AM_HAL_STIMER_CFG_CLEAR | AM_HAL_STIMER_CFG_FREEZE);
  am_hal_stimer_config(AM_HAL_STIMER_HFRC_3MHZ);

  //Turn on ADC
  ap3_adc_setup();

  //Set any pinModes
  pinMode(ledPin, OUTPUT);

  //Turn on Serial
  Serial.begin(115200);
  delay(10);
  Serial.println("Back on");

  //Turn on I2C
  Wire.begin();

  //Restart Sensors
  Serial.println("restarted!");
//  if (barometricSensor.begin() == false)
//  {
//    Serial.println("MS5637 sensor did not respond. Please check wiring.");
//  }
}

//Called once number of milliseconds has passed
extern "C" void am_stimer_cmpr6_isr(void)
{
  uint32_t ui32Status = am_hal_stimer_int_status_get(false);
  if (ui32Status & AM_HAL_STIMER_INT_COMPAREG)
  {
    am_hal_stimer_int_clear(AM_HAL_STIMER_INT_COMPAREG);
  }
}
