
// DEV NOTES: https://forum.sparkfun.com/viewtopic.php?t=52431

// WATCHDOG TIMER CONFIGURATION:
void configureWdt(){
  am_hal_wdt_config_t g_sWatchdogConfig = {
  //Substitude other values for AM_HAL_WDT_LFRC_CLK_16HZ to increase/decrease the range
  .ui32Config = AM_HAL_WDT_LFRC_CLK_16HZ | AM_HAL_WDT_ENABLE_RESET | AM_HAL_WDT_ENABLE_INTERRUPT, // Configuration values for generated watchdog timer event.
  //****** EVEN THOUGH THESE IMPLY 16-BIT, THEY ARE ONLY 8-BIT - 255 MAX!
  .ui16InterruptCount = 128, //MAX 255! // Set WDT interrupt timeout for 5 seconds (128 / 16 = 8).  // Number of watchdog timer ticks allowed before a watchdog interrupt event is generated.
  .ui16ResetCount = 240 //MAX 255! // Set WDT reset timeout for 15 seconds (240 / 16 = 10).  // Number of watchdog timer ticks allowed before the watchdog will issue a system reset.
  };
  am_hal_clkgen_control(AM_HAL_CLKGEN_CONTROL_LFRC_START, 0); // LFRC must be turned on for this example as the watchdog only runs off of the LFRC.
  am_hal_wdt_init(&g_sWatchdogConfig);  // Configure the watchdog.
  NVIC_EnableIRQ(WDT_IRQn); // Enable the interrupt for the watchdog in the NVIC.
  am_hal_interrupt_master_enable();
  am_hal_wdt_start(); // Enable the watchdog.
}



////void startWatchdogTimer(){ 
//  am_hal_clkgen_control(AM_HAL_CLKGEN_CONTROL_LFRC_START, 0); // LFRC must be turned on for this example as the watchdog only runs off of the LFRC.
//  am_hal_wdt_init(&g_sWatchdogConfig);  // Configure the watchdog.
//  NVIC_EnableIRQ(WDT_IRQn); // Enable the interrupt for the watchdog in the NVIC.
//  am_hal_interrupt_master_enable();
//  am_hal_wdt_start(); // Enable the watchdog.
////}

// RESET WATCHDOG TIMER
void petDog(){
  am_hal_wdt_restart();
  wdtFlag = false;
  wdtCounter = 0;
}

void configureSleepStimer(){
    // Setup interrupt to trigger when the number of ms have elapsed AMOUNT OF TIME HERE
    am_hal_stimer_int_clear(AM_HAL_STIMER_INT_COMPAREG); //Clear CT6
    am_hal_stimer_int_enable(AM_HAL_STIMER_INT_COMPAREG); //Enable C/T G=6
    //am_hal_stimer_config(AM_HAL_STIMER_CFG_COMPARE_G_ENABLE);
    am_hal_stimer_compare_delta_set(6, sysTicksToSleep); // enter time here
    NVIC_EnableIRQ(STIMER_CMPR6_IRQn);
    stimerFlag = false;
}

void configureLogAlarm() {
  // Alarm modes:
  // 0: Alarm interrupt disabled
  // 1: Alarm match hundredths, seconds, minutes, hour, day, month  (every year)
  // 2: Alarm match hundredths, seconds, minutes, hours, day        (every month)
  // 3: Alarm match hundredths, seconds, minutes, hours, weekday    (every week)
  // 4: Alarm match hundredths, seconds, minute, hours              (every day)
  // 5: Alarm match hundredths, seconds, minutes                    (every hour)
  // 6: Alarm match hundredths, seconds                             (every minute)
  // 7: Alarm match hundredths                                      (every second)

  // Manually set the RTC date and time
    rtc.setTime(13, 0, 0, 0, 1, 1, 21); // 12:59:50.000, June 3rd, 2020 (hund, ss, mm, hh, dd, mm, yy)
    rtc.setAlarm(0, secondsLog, 0, 13, 3, 6); // 13:00:00.000, June 3rd (hund, ss, mm, hh, dd, mm). Note: No year alarm register

  // Get time before starting rolling alarm
  //rtc.getTime();

  // Set the alarm mode
    rtc.setAlarmMode(6);

  // Attach alarm interrupt
    rtc.attachInterrupt();

  // Clear the RTC alarm interrupt
    am_hal_rtc_int_clear(AM_HAL_RTC_INT_ALM);

  // Clear alarm flag
    alarmFlag = false;
}
