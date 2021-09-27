/*
*/

#include <Wire.h>
#include <SD.h>
#include <SPI.h>

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
int count = 0; //counter
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
  myFile = SD.open("TEST.txt", FILE_WRITE);
  if(!myFile)
  {
    Serial.println(F("Failed to create UBX data file! Freezing..."));
    while (1);
  }
}

void loop(void) {
  digitalWrite(ledPin, HIGH);

  if (millis() > interval){
    Serial.println("timeout, going to sleep");
    digitalWrite(ledPin, LOW);
    delay(1000);

    //myFile.write(int(millis()));
    myFile.write("Logging Stopped");
    delay(10);
    myFile.close();
    lastPrint = 0;
    Serial.println("logging stopped");
    Serial.flush();
    delay(100);
    goToSleep();
  }

  //DO GNSS STUFF
  if (millis() > (lastPrint + 1000)){ // Print bytesWritten once per second
    Serial.print(count);
    myFile.print(count);
    count++;
    lastPrint = millis();
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
  beginSD();

  if (!SD.begin(sdChipSelect)){
    Serial.println("Card failed, or not present. Freezing...");
    // don't do anything more:
    while (1);
  }
  myFile = SD.open("TEST.txt", FILE_WRITE);
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

//Called once number of milliseconds has passed
extern "C" void am_stimer_cmpr6_isr(void)
{
  uint32_t ui32Status = am_hal_stimer_int_status_get(false);
  if (ui32Status & AM_HAL_STIMER_INT_COMPAREG)
  {
    am_hal_stimer_int_clear(AM_HAL_STIMER_INT_COMPAREG);
  }
}
