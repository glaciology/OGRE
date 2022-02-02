
// DEV NOTES: https://forum.sparkfun.com/viewtopic.php?t=52431

// WATCHDOG TIMER CONFIGURATION/START:
void configureWdt(){
  // Settings:
  am_hal_wdt_config_t g_sWatchdogConfig = {
  //Substitude other values for AM_HAL_WDT_LFRC_CLK_16HZ to increase/decrease the range
  .ui32Config = AM_HAL_WDT_LFRC_CLK_16HZ | AM_HAL_WDT_ENABLE_RESET | AM_HAL_WDT_ENABLE_INTERRUPT, // Configuration values/clk
  //****** EVEN THOUGH THESE IMPLY 16-BIT, THEY ARE ONLY 8-BIT - 255 MAX!
  .ui16InterruptCount = 160, //MAX 255! // Set WDT interrupt for 10 seconds (160 / 16 = 10). 
  .ui16ResetCount = 240 //MAX 255! // Set WDT reset for 15 seconds (240 / 16 = 15).  
  };
  
  // STARTING TIMER
  am_hal_clkgen_control(AM_HAL_CLKGEN_CONTROL_LFRC_START, 0); // LFRC must be turned on as the watchdog requires LFRC.
  am_hal_wdt_init(&g_sWatchdogConfig);  // Configure the watchdog.
  NVIC_EnableIRQ(WDT_IRQn); // Enable the interrupt for the watchdog in the NVIC.
  am_hal_interrupt_master_enable();
  am_hal_wdt_start(); // Enable the watchdog.
}

// RESET WATCHDOG TIMER
void petDog(){
  am_hal_wdt_restart();
  wdtFlag = false;
  wdtCounter = 0;
}

// RTC CONFIGURATION/START:
void configureLogAlarm() {
  // Alarm modes:
  /*
   0: Alarm interrupt disabled
   1: Alarm match hundredths, seconds, minutes, hour, day, month  (every year)
   2: Alarm match hundredths, seconds, minutes, hours, day        (every month)
   3: Alarm match hundredths, seconds, minutes, hours, weekday    (every week)
   4: Alarm match hundredths, seconds, minute, hours              (every day)
   5: Alarm match hundredths, seconds, minutes                    (every hour)
   6: Alarm match hundredths, seconds                             (every minute)
   7: Alarm match hundredths                                      (every second)
  */
  
  am_hal_rtc_int_clear(AM_HAL_RTC_INT_ALM); // Clear the RTC alarm interrupt

  // This should be set by UBLOX in FUTURE
  rtc.setTime(13, 0, 0, 0, 1, 1, 21); // 13:00:00.000, June 3rd, 2020
    
  if (logMode == 2){
    rtc.setAlarm(13, minutesLog, secondsLog, 0, 1, 1); // 13:00:00.000, June 3rd. Note: No year alarm register
    rtc.setAlarmMode(6);
  }
  
  rtc.attachInterrupt();
  alarmFlag = false;
}

void configureSleepAlarm() {

  // Clear the RTC alarm interrupt
  am_hal_rtc_int_clear(AM_HAL_RTC_INT_ALM);

  // This should be set by UBLOX in FUTURE
  rtc.setTime(13, 0, 0, 0, 1, 1, 21); // 13:00:00.000, June 3rd, 2020 
    
  if (logMode == 2){
    DEBUG_PRINTLN(F("Info: Setting rolling RTC Sleep alarm"));
    rtc.setAlarm(13, minutesSleep, secondsSleep, 0, 1, 1); // 13:00:00.000, June 3rd. Note: No year alarm register
    rtc.setAlarmMode(6);
  }
    
  rtc.attachInterrupt();
  alarmFlag = false;
}
