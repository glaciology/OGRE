
// POWER DOWN AND WAIT FOR INTERRUPT
void goToSleep() {
  # if DEBUG
    Serial.end();
  #endif 
  myWire.end(); //Power down I2C
  mySpi.end(); //Power down SPI
  power_adc_disable();
  //powerControlADC(false); //Power down ADC. It it started by default before setup().
  digitalWrite(LED, LOW); // Turn off LED
  zedPowerOff(); 
  peripheralPowerOff();
  
  // Force peripherals off
  am_hal_pwrctrl_periph_disable(AM_HAL_PWRCTRL_PERIPH_IOM0);
  am_hal_pwrctrl_periph_disable(AM_HAL_PWRCTRL_PERIPH_IOM1);
  am_hal_pwrctrl_periph_disable(AM_HAL_PWRCTRL_PERIPH_IOM2);
  am_hal_pwrctrl_periph_disable(AM_HAL_PWRCTRL_PERIPH_IOM3);
  am_hal_pwrctrl_periph_disable(AM_HAL_PWRCTRL_PERIPH_IOM4);
  am_hal_pwrctrl_periph_disable(AM_HAL_PWRCTRL_PERIPH_IOM5);
  am_hal_pwrctrl_periph_disable(AM_HAL_PWRCTRL_PERIPH_ADC);
  am_hal_pwrctrl_periph_disable(AM_HAL_PWRCTRL_PERIPH_UART0);
  am_hal_pwrctrl_periph_disable(AM_HAL_PWRCTRL_PERIPH_UART1);

  // Disable all pads except 16, 18, 34 (LED and PWR CNTRLS)
  for (int x = 0; x < 50; x++)
  {
    if ((x != LED) && (x != ZED_POWER) && (x != PERIPHERAL_POWER))
    {
      am_hal_gpio_pinconfig(x, g_AM_HAL_GPIO_DISABLE);
    }
  }

  zedPowerOff();
  peripheralPowerOff(); 

  // Use the lower power 32kHz clock.
  am_hal_stimer_config(AM_HAL_STIMER_CFG_CLEAR | AM_HAL_STIMER_CFG_FREEZE);
  am_hal_stimer_config(AM_HAL_STIMER_XTAL_32KHZ);
  //am_hal_stimer_config(AM_HAL_STIMER_XTAL_32KHZ | AM_HAL_STIMER_CFG_COMPARE_G_ENABLE);
  
  //Power down cache, flash, SRAM
  am_hal_pwrctrl_memory_deepsleep_powerdown(AM_HAL_PWRCTRL_MEM_ALL); // Power down all flash and cache
  am_hal_pwrctrl_memory_deepsleep_retain(AM_HAL_PWRCTRL_MEM_SRAM_384K); // Retain all SRAM

  //Deep Sleep
  am_hal_sysctrl_sleep(AM_HAL_SYSCTRL_SLEEP_DEEP);

  ///////// Waiting for RTC or WDT Interrupt 
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

  // Turn on ADC
  ap3_adc_setup();
  //powerControlADC(true); // Turn on ADC
  myWire.begin(); // I2C
  disableI2CPullups();
  mySpi.begin(); // SPI
  
  petDog();

  //Turn on Serial
  #if DEBUG
    Serial.begin(115200); // open serial port
  #endif
  delay(2500);

}

///////// AUXILIARLY OFF/ON FUNCTIONS
void zedPowerOff() {
  digitalWrite(ZED_POWER, HIGH);
}

void peripheralPowerOff() {
  delay(250); 
  digitalWrite(PERIPHERAL_POWER, HIGH);
}

void zedPowerOn() {
  digitalWrite(ZED_POWER, LOW);
  delay(250);
}

void peripheralPowerOn() {
  digitalWrite(PERIPHERAL_POWER, LOW);
  delay(250); 
  
}

void disableI2CPullups() {
  #if (apolloCore == 2) 
    ///////// On Apollo3 v2 MANUALLY DISABLE PULLUPS - IOM and pin #s specific to Artemis MicroMod
    am_hal_gpio_pincfg_t sclPinCfg = g_AM_BSP_GPIO_IOM4_SCL;  // Artemis MicroMod Processor Board uses IOM4 for I2C communication
    am_hal_gpio_pincfg_t sdaPinCfg = g_AM_BSP_GPIO_IOM4_SDA;  //
    sclPinCfg.ePullup = AM_HAL_GPIO_PIN_PULLUP_NONE;          // Disable the SCL/SDA pull-ups
    sdaPinCfg.ePullup = AM_HAL_GPIO_PIN_PULLUP_NONE;          //
    pin_config(SCL, sclPinCfg);                               // Artemis MicroMod Processor Board uses Pin/Pad 39 for SCL
    pin_config(SDA, sdaPinCfg);                               // Artemis MicroMod Processor Board uses Pin/Pad 40 for SDA
  #else 
    myWire.setPullups(0);
    myWire.setClock(400000); 
  #endif

}

// Non-blocking blink LED (https://forum.arduino.cc/index.php?topic=503368.0)
void blinkLed(byte ledFlashes, unsigned int leddelay) {
  byte i = 0;
  while (i < ledFlashes * 2)
  {
    unsigned long currMillis = millis();
    if (currMillis - prevMillis >= leddelay)
    {
      digitalWrite(LED, !digitalRead(LED));
      prevMillis = currMillis;
      i++;
    }
  }
  // Turn off LED
  digitalWrite(LED, LOW);
}
