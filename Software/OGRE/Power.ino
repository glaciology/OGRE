/* the Low Power functions are adapted from Sparkfun examples LowPower and OpenLog platform. 
 *  and Example6_LowPower_Alarm. 
 *  Sync RTC functionality (sync, getDateTime, printAlarm) modified from A. Garbo GVT.
 *  See license for more details. 
 */

void initializeBuses() {
  pinMode(ZED_POWER, OUTPUT);
  zedPowerOn();
  myWire.begin();
  disableI2CPullups();
  pinMode(PER_POWER, OUTPUT);
  peripheralPowerOn();
  mySpi.begin();
  delay(1);
}


void deinitializeBuses() {
  online.rtcSync = false; // This line must be at top! 
  
  if ((logMode == 2 || summerInterval == true) && online.chargedBat == true) { 
    DEBUG_PRINTLN("Info: LOG MODE 2 or SUMMER... No Deinitialize."); 
    return;
  }
  
  myWire.end();           // Power down I2C
  mySpi.end();            // Power down SPI
  zedPowerOff();
  peripheralPowerOff();

  online.gnss = false;   // Clear online/offline flags
  online.uSD = false;
  closeFailCounter = 0;
  lowBatteryCounter = 0;
  
}

// POWER DOWN AND WAIT FOR INTERRUPT
void goToSleep() { // Function from Sparkfun Example6_LowPower_Alarm.ino, see License/Readme.
  if ((logMode == 2 || summerInterval == true) && online.chargedBat == true) { // continuous mode or during summer in mode 6: no sleep
    DEBUG_PRINTLN("Info: LOG MODE 2 or SUMMER... No Sleep."); 
    return;
  }
  
# if DEBUG
  Serial.end();
#endif

  power_adc_disable();    // Disable ADC
  digitalWrite(LED, LOW); // Turn off LED
  #if HARDWARE_VERSION == 2
    digitalWrite(LED2, LOW); // Turn off LED
  #endif

  // Turn peripherals off
  am_hal_pwrctrl_periph_disable(AM_HAL_PWRCTRL_PERIPH_IOM0);
  am_hal_pwrctrl_periph_disable(AM_HAL_PWRCTRL_PERIPH_IOM1);
  am_hal_pwrctrl_periph_disable(AM_HAL_PWRCTRL_PERIPH_IOM2);
  am_hal_pwrctrl_periph_disable(AM_HAL_PWRCTRL_PERIPH_IOM3);
  am_hal_pwrctrl_periph_disable(AM_HAL_PWRCTRL_PERIPH_IOM4);
  am_hal_pwrctrl_periph_disable(AM_HAL_PWRCTRL_PERIPH_IOM5);
  am_hal_pwrctrl_periph_disable(AM_HAL_PWRCTRL_PERIPH_ADC);
  am_hal_pwrctrl_periph_disable(AM_HAL_PWRCTRL_PERIPH_UART0);
  am_hal_pwrctrl_periph_disable(AM_HAL_PWRCTRL_PERIPH_UART1);

  // Disable all pads except LED and PWR CONTROL PINS
  for (int x = 0; x < 50; x++)
  {
    if ((x != LED) && (x != ZED_POWER) && (x != PER_POWER) )
    {
      am_hal_gpio_pinconfig(x, g_AM_HAL_GPIO_DISABLE);
    }
  }

  // Use the lower power 32kHz clock.
  am_hal_stimer_config(AM_HAL_STIMER_CFG_CLEAR | AM_HAL_STIMER_CFG_FREEZE);
  am_hal_stimer_config(AM_HAL_STIMER_XTAL_32KHZ);

  //Power down cache, flash, SRAM
  am_hal_pwrctrl_memory_deepsleep_powerdown(AM_HAL_PWRCTRL_MEM_ALL); // Power down all flash and cache
  am_hal_pwrctrl_memory_deepsleep_retain(AM_HAL_PWRCTRL_MEM_SRAM_384K); // Retain all SRAM

  //Deep Sleep
  am_hal_sysctrl_sleep(AM_HAL_SYSCTRL_SLEEP_DEEP);

  ///////// Waiting for RTC or WDT Interrupt
  ///////// Sleeping...
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

  petDog();

#if DEBUG
  Serial.begin(115200); 
  //delay(2500);
#endif

  if (measureBattery == true) {
    ap3_adc_setup();
    ap3_set_pin_to_analog(BAT);
  }
}

///////// AUXILIARLY OFF/ON FUNCTIONS
void zedPowerOff() {
  if (HARDWARE_VERSION == 1) {
    digitalWrite(ZED_POWER, LOW);
  } else {
    digitalWrite(ZED_POWER, HIGH);
  }
}


void peripheralPowerOff() {
  delay(250);
  if (HARDWARE_VERSION == 1) {
    digitalWrite(PER_POWER, LOW);
  } else {
    digitalWrite(PER_POWER, HIGH);
  }
}


void zedPowerOn() {
  if (HARDWARE_VERSION == 1) {
    digitalWrite(ZED_POWER, HIGH);
  } else {
    digitalWrite(ZED_POWER, LOW);
  }
  delay(1000);
}


void peripheralPowerOn() {
  if (HARDWARE_VERSION == 1) {
    digitalWrite(PER_POWER, HIGH);
  } else {
    digitalWrite(PER_POWER, LOW);
  }
  delay(500);
}


void disableI2CPullups() {
  myWire.setPullups(0);
  myWire.setClock(100000);
}


void enableI2CPullups() {
  myWire.setPullups(24);
}


float measBat() {
  analogReadResolution(14);  //Set resolution to 14 bit
  pinMode(BAT_CNTRL, OUTPUT);
  digitalWrite(BAT_CNTRL, HIGH);
  delay(1);
  int measure0 = analogRead(BAT);
  delay(10);
  int measure1 = analogRead(BAT);
  delay(1);
  digitalWrite(BAT_CNTRL, LOW);
  float vcc0 = (float)measure0 * gain / 16384.0 + offset; // convert to normal number
  float vcc1 = (float)measure1 * gain / 16384.0 + offset; // convert to normal number
  float voltageFinal = (vcc0 + vcc1) / 2;
  return voltageFinal;
}


void checkBattery() {
  if (measureBattery == true) {
    if (measBat() < shutdownThreshold) {
      DEBUG_PRINTLN("Info: BATTERY LOW. SLEEPING");
      online.chargedBat = false;
      pinMode(PER_POWER, OUTPUT);  // define as OUTPUT in case pins haven't been initialized
      pinMode(ZED_POWER, OUTPUT);
      deinitializeBuses(); // turn off GNSS, etc.
      while((measBat() < shutdownThreshold + .5)) { // IMPORTANT - RECHARGE THRESHOLD SET HERE
        goToSleep();
        petDog(); 
        DEBUG_PRINT("Info: Battery: "); DEBUG_PRINTLN(measBat());
      }
      DEBUG_PRINTLN("Info: Battery Charged."); 
      lowBatteryCounter++;
      online.chargedBat = true;
    }
  }
}

// blink LED (https://forum.arduino.cc/index.php?topic=503368.0)
void blinkLed(byte ledFlashes, unsigned int ledDelay, LedColor color) {
  byte i = 0;
  byte ledPin;

    #if HARDWARE_VERSION == 1
        // v1 hardware: ignore color, always use the single LED
        ledPin = LED;
    #elif HARDWARE_VERSION == 2
        // v2 hardware: choose pin based on requested color
        ledPin = (color == GREEN) ? LED : LED2;
    #endif

  while (i < ledFlashes * 2) {
    unsigned long currMillis = millis();
    if (currMillis - prevMillis >= ledDelay) {
      digitalWrite(ledPin, !digitalRead(ledPin));
      prevMillis = currMillis;
      i++;
    }
  }
  // Turn off LED
  digitalWrite(ledPin, LOW);
}

// Used to read reset codes from status register memory
String getStatusCodes(uint32_t stat_value) {
  String statusCodes = "";
  
  if ((stat_value >> 31) & 0x1) {
    statusCodes += "SBOOT ";
  }
  if ((stat_value >> 30) & 0x1) {
    statusCodes += "FBOOT ";
  }
  if ((stat_value >> 10) & 0x1) {
    statusCodes += "BOBSTAT ";
  }
  if ((stat_value >> 9) & 0x1) {
    statusCodes += "BOFSTAT ";
  }
  if ((stat_value >> 8) & 0x1) {
    statusCodes += "BOCSTAT ";
  }
  if ((stat_value >> 7) & 0x1) {
    statusCodes += "BOUSTAT ";
  }
  if ((stat_value >> 6) & 0x1) {
    statusCodes += "WDRSTAT ";
  }
  if ((stat_value >> 5) & 0x1) {
    statusCodes += "DBGRSTAT ";
  }
  if ((stat_value >> 4) & 0x1) {
    statusCodes += "POIRSTAT ";
  }
  if ((stat_value >> 3) & 0x1) {
    statusCodes += "SWRSTAT ";
  }
  if ((stat_value >> 2) & 0x1) {
    statusCodes += "BORSTAT ";
  }
  if ((stat_value >> 1) & 0x1) {
    statusCodes += "PORSTAT ";
  }
  if (stat_value & 0x1) {
    statusCodes += "EXRSTAT ";
  }
  
  return statusCodes;
}
