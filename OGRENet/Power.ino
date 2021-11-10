
// POWER DOWN AND WAIT FOR INTERRUPT
void goToSleep() {

  Wire.end(); //Power down I2C
  SPI.end(); //Power down SPI
  Serial.end();
  powerControlADC(false); //Power down ADC. It it started by default before setup().
  digitalWrite(ledPin, LOW); // Turn off LED
  qwiicPowerOff();
  //peripheralPowerOff();

  // We use counter/timer 6 to cause us to wake up from sleep but 0 to 7 are available
  // CT 7 is used for Software Serial. All CTs are used for Servo.
  am_hal_stimer_int_clear(AM_HAL_STIMER_INT_COMPAREG); //Clear CT6
  am_hal_stimer_int_enable(AM_HAL_STIMER_INT_COMPAREG); //Enable C/T G=6

   // Use the lower power 32kHz clock. Use it to run CT6 as well.
  am_hal_stimer_config(AM_HAL_STIMER_CFG_CLEAR | AM_HAL_STIMER_CFG_FREEZE);
  am_hal_stimer_config(AM_HAL_STIMER_XTAL_32KHZ | AM_HAL_STIMER_CFG_COMPARE_G_ENABLE);

  // Setup interrupt to trigger when the number of ms have elapsed AMOUNT OF TIME HERE *********
  am_hal_stimer_compare_delta_set(6, sysTicksToSleep);
  
  //Power down cache, flash, SRAM
  am_hal_pwrctrl_memory_deepsleep_powerdown(AM_HAL_PWRCTRL_MEM_ALL); // Power down all flash and cache
  am_hal_pwrctrl_memory_deepsleep_retain(AM_HAL_PWRCTRL_MEM_SRAM_384K); // Retain all SRAM

  // Enable the timer interrupt in the NVIC.
  NVIC_EnableIRQ(STIMER_CMPR6_IRQn);

  //Deep Sleep
  am_hal_sysctrl_sleep(AM_HAL_SYSCTRL_SLEEP_DEEP);
  
  //Turn off interrupt
  NVIC_DisableIRQ(STIMER_CMPR6_IRQn);
  am_hal_stimer_int_disable(AM_HAL_STIMER_INT_COMPAREG); //Disable C/T G=6

  ///////// Sleeping
  ///////// Sleeping
  ///////// Sleeping...

  // WAKE
  wakeFromSleep();
}


// POWER UP SMOOTHLY
void wakeFromSleep() {

  am_hal_pwrctrl_memory_deepsleep_powerdown(AM_HAL_PWRCTRL_MEM_MAX);

  // Go back to using the main clock
  am_hal_stimer_config(AM_HAL_STIMER_CFG_CLEAR | AM_HAL_STIMER_CFG_FREEZE);
  am_hal_stimer_config(AM_HAL_STIMER_HFRC_3MHZ);

  //Turn on ADC
  powerControlADC(true); // Turn on ADC
  Wire.begin(); // I2C
  SPI.begin(); // SPI
  qwiicPowerOn();
  //peripheralPowerOn();

  //Set any pinModes
  digitalWrite(ledPin, HIGH);

  //Turn on Serial
  Serial.begin(115200);
  delay(2500);
  Serial.println("Turning back on");

  //CONFIM SD CARD RESTARTED
  if (!SD.begin(PIN_SD_CS)) {
    Serial.println("Card failed, or not present. Freezing...");
    // don't do anything more:
    while (1);
  }
  myFile = SD.open("RAWX.UBX", FILE_WRITE);
  delay(100);

  //CONFIRM QWIIC GNSS RESTARTED
  myGNSS.setFileBufferSize(fileBufferSize); // setFileBufferSize must be called _before_ .begin
  if (myGNSS.begin() == false) {
    Serial.println(F("u-blox GNSS not detected at default I2C address. Please check wiring. Freezing..."));
    while (1);
  }
  Serial.println("restarted");
  PREVIOUS_MILLIS = millis();
  Serial.println(PREVIOUS_MILLIS);
}



///////// AUXILIARLY OFF/ON FUNCTIONS
void qwiicPowerOff() {
  digitalWrite(PIN_QWIIC_POWER, LOW);
}

//void peripheralPowerOff() {
//  delay(250); // Non-blocking delay
//  digitalWrite(PIN_PWC_POWER, LOW);
//}

void qwiicPowerOn() {
  digitalWrite(PIN_QWIIC_POWER, HIGH);
  delay(10);
}

//void peripheralPowerOn() {
//  digitalWrite(PIN_PWC_POWER, HIGH);
//  delay(250); // Non-blocking delay to allow Qwiic devices time to power up
//}
