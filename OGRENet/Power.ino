
// POWER DOWN AND WAIT FOR INTERRUPT
void goToSleep() {

  Wire.end(); //Power down I2C
  SPI.end(); //Power down SPI
  Serial.end();
  powerControlADC(false); //Power down ADC. It it started by default before setup().
  digitalWrite(LED, LOW); // Turn off LED
  qwiicPowerOff();
  //peripheralPowerOff();

   // Use the lower power 32kHz clock. Use it to run CT6 as well.
  am_hal_stimer_config(AM_HAL_STIMER_CFG_CLEAR | AM_HAL_STIMER_CFG_FREEZE);
  am_hal_stimer_config(AM_HAL_STIMER_XTAL_32KHZ | AM_HAL_STIMER_CFG_COMPARE_G_ENABLE);

  //  *********
  configureSleepStimer();
  
  //Power down cache, flash, SRAM
  am_hal_pwrctrl_memory_deepsleep_powerdown(AM_HAL_PWRCTRL_MEM_ALL); // Power down all flash and cache
  am_hal_pwrctrl_memory_deepsleep_retain(AM_HAL_PWRCTRL_MEM_SRAM_384K); // Retain all SRAM

  //Deep Sleep
  am_hal_sysctrl_sleep(AM_HAL_SYSCTRL_SLEEP_DEEP);

  ///////// Waiting for Stimer Interrupt 
  ///////// Sleeping
  ///////// Sleeping...
  
  //Turn off interrupt
  NVIC_DisableIRQ(STIMER_CMPR6_IRQn);
  am_hal_stimer_int_disable(AM_HAL_STIMER_INT_COMPAREG); //Disable C/T G=6

  // WAKE
  wakeFromSleep();
}


// POWER UP SMOOTHLY
void wakeFromSleep() {

  am_hal_pwrctrl_memory_deepsleep_powerdown(AM_HAL_PWRCTRL_MEM_MAX);

  // Go back to using the main clock
  am_hal_stimer_config(AM_HAL_STIMER_CFG_CLEAR | AM_HAL_STIMER_CFG_FREEZE);
  am_hal_stimer_config(AM_HAL_STIMER_HFRC_3MHZ);

  // Turn on ADC
  powerControlADC(true); // Turn on ADC
  Wire.begin(); // I2C
  qwiicPowerOn();
  //peripheralPowerOn();
  SPI.begin(); // SPI

  //Turn on Serial
  #if DEBUG
    Serial.begin(115200); // open serial port
  #endif
  delay(2500);

//  // RECONFIGURATIONS
//  configureSD();
//  configureGNSS();
//  configureLogAlarm();
  
  DEBUG_PRINTLN("restarted");

}


///////// AUXILIARLY OFF/ON FUNCTIONS
void qwiicPowerOff() {
  digitalWrite(PIN_ZED_POWER, LOW);
}

void peripheralPowerOff() {
  delay(250); // Non-blocking delay
  digitalWrite(PIN_PERIPHERAL, LOW);
}

void qwiicPowerOn() {
  digitalWrite(PIN_ZED_POWER, HIGH);
  delay(10);
}

void peripheralPowerOn() {
  digitalWrite(PIN_PERIPHERAL, HIGH);
  delay(250); 
  
}

void disableI2CPullups() {
    ///////// On Apollo3 v2 MANUALLY DISABLE PULLUPS - IOM and pin #s specific to Artemis MicroMod
  am_hal_gpio_pincfg_t sclPinCfg = g_AM_BSP_GPIO_IOM4_SCL;  // Artemis MicroMod Processor Board uses IOM4 for I2C communication
  am_hal_gpio_pincfg_t sdaPinCfg = g_AM_BSP_GPIO_IOM4_SDA;  //
  sclPinCfg.ePullup = AM_HAL_GPIO_PIN_PULLUP_NONE;          // Disable the SCL/SDA pull-ups
  sdaPinCfg.ePullup = AM_HAL_GPIO_PIN_PULLUP_NONE;          //
  pin_config(PinName(39), sclPinCfg);                       // Artemis MicroMod Processor Board uses Pin/Pad 39 for SCL
  pin_config(PinName(40), sdaPinCfg);                       // Artemis MicroMod Processor Board uses Pin/Pad 40 for SDA
}




unsigned long previousMillis      = 0; 
unsigned long currentMillis = millis();

// Non-blocking blink LED (https://forum.arduino.cc/index.php?topic=503368.0)
void blinkLed(byte ledFlashes, unsigned int ledDelay) {
  byte i = 0;
  while (i < ledFlashes * 2)
  {
    unsigned long currentMillis = millis();
    if (currentMillis - previousMillis >= ledDelay)
    {
      digitalWrite(LED, !digitalRead(LED));
      previousMillis = currentMillis;
      i++;
    }
  }
  // Turn off LED
  digitalWrite(LED, LOW);
}
