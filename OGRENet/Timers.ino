
// DEV NOTES: https://forum.sparkfun.com/viewtopic.php?t=52431

void configureWdt(){
//  // SETUP:
//  am_hal_wdt_config_t g_sWatchdogConfig = {
//  ////Substitude other values for AM_HAL_WDT_LFRC_CLK_16HZ to increase/decrease the range
//  .ui32Config = AM_HAL_WDT_LFRC_CLK_16HZ | AM_HAL_WDT_ENABLE_RESET | AM_HAL_WDT_ENABLE_INTERRUPT, // Configuration values/clk
//  ////****** EVEN THOUGH THESE IMPLY 16-BIT, THEY ARE ONLY 8-BIT - 255 MAX!
//  .ui16InterruptCount = 160,                                  //MAX 255! Set WDT interrupt for 10 seconds (160 / 16 = 10). 
//  .ui16ResetCount = 240                                       //MAX 255! Set WDT reset for 15 seconds (240 / 16 = 15).  
//  };
//  
//  // STARTING TIMER
//  am_hal_clkgen_control(AM_HAL_CLKGEN_CONTROL_LFRC_START, 0); // LFRC must be turned on as the watchdog requires LFRC.
//  am_hal_wdt_init(&g_sWatchdogConfig);                        // Initialize the watchdog.
//  NVIC_EnableIRQ(WDT_IRQn);                                   // Enable the interrupt for the watchdog in the NVIC.
//  am_hal_interrupt_master_enable();
//  am_hal_wdt_start();                                         // Enable the watchdog.

  wdt.configure(WDT_16HZ, 128, 240); // 16 Hz clock, 10-second interrupt period, 15-second reset period

  // Start the watchdog timer
  wdt.start();
}


// RESET WATCHDOG TIMER
void petDog() {
  //am_hal_wdt_restart();
  wdt.restart();
  wdtFlag = false;
  wdtCounter = 0;
}


// RTC CONFIGURATION/START:
void configureLogAlarm() {
  /* ALARM MODES
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

  // 1 = daily at defined hour, 2 = test mode, 3 = continuous, 4 = monthly for 24 hr
  
  if (logMode == 1) {
    rtc.setAlarm(logEndHr, 0, 0, 0, 0, 0); 
    rtc.setAlarmMode(4); // match every day
    rtc.attachInterrupt();
  }
  
  if (logMode == 2) {
    rtc.setTime(13, 0, 0, 0, 1, 1, 21); // 13:00:00.000, 1/1/21
    rtc.setAlarm(13, 0, secondsLog, 0, 1, 1); 
    rtc.setAlarmMode(6);
    rtc.attachInterrupt();
  }

  if (logMode == 3){ // empty - continuous
  }

  if (logMode == 4) {
    // WILL LOG FOR 24 HOURS ON SPECIFIED DAY OF MONTH
    rtc.setAlarm(23, 59, 59, 0, 0, 0); 
    rtc.setAlarmMode(2);
    rtc.attachInterrupt();
  }

  alarmFlag = false;
  
}


void configureSleepAlarm() {

  // Clear the RTC alarm interrupt
  am_hal_rtc_int_clear(AM_HAL_RTC_INT_ALM);
  
  if (logMode == 1){
    rtc.setAlarm(logStartHr, 0, 0, 0, 0, 0);
    rtc.setAlarmMode(4); // match every day
    }
    
  if (logMode == 2){
    rtc.setTime(13, 0, 0, 0, 1, 1, 21); // 13:00:00.000, June 3rd, 2020 
    rtc.setAlarm(13, 0, secondsSleep, 0, 1, 1); // No year alarm register
    rtc.setAlarmMode(6);
  }

  if (logMode == 4){
    rtc.setAlarm(0, 0, 0, 0, logStartDay, 0); 
    rtc.setAlarmMode(2); // match every month
    
  }
  rtc.attachInterrupt();
  alarmFlag = false;
}


void syncRtc() {

    unsigned long loopStartTime = millis();
    
   // Check u-blox GNSS initialized successfully
   if (online.gnss) {
     rtcSyncFlag = false;
  
     DEBUG_PRINTLN("Info: Attempting to synchronize RTC with GNSS...");
  
     // Attempt to acquire a valid GNSS position fix for up to 3 minutes
     while (!rtcSyncFlag && millis() - loopStartTime < 3 * 60UL * 1000UL) {
       petDog(); 
  
       // Check for UBX-NAV-PVT messages
       if (gnss.getPVT()) {
         digitalWrite(LED, !digitalRead(LED)); // Blink LED
  
         bool dateValidFlag = gnss.getConfirmedDate();
         bool timeValidFlag = gnss.getConfirmedTime();
         byte fixType = gnss.getFixType();
  
#if DEBUG_GNSS
         char gnssBuffer[100];
         sprintf(gnssBuffer, "%04u-%02d-%02d %02d:%02d:%02d.%03d,%ld,%ld,%d,%d,%d,%d,%d",
                 gnss.getYear(), gnss.getMonth(), gnss.getDay(),
                 gnss.getHour(), gnss.getMinute(), gnss.getSecond(), gnss.getMillisecond(),
                 gnss.getLatitude(), gnss.getLongitude(), gnss.getSIV(),
                 gnss.getPDOP(), gnss.getFixType(),
                 dateValidFlag, timeValidFlag);
         DEBUG_PRINTLN(gnssBuffer);
#endif
  
          // Check if date and time are valid and sync RTC with GNSS
          if (fixType == 3 && dateValidFlag && timeValidFlag) {
            unsigned long rtcEpoch = rtc.getEpoch();        // Get RTC epoch time
            unsigned long gnssEpoch = gnss.getUnixEpoch();  // Get GNSS epoch time
            rtc.setEpoch(gnssEpoch);                        // Set RTC date and time
            rtcDrift = gnssEpoch - rtcEpoch;                // Calculate RTC drift (debug)
            rtcSyncFlag = true;                             // Set flag, end SYNC
  
            DEBUG_PRINT("Info: RTC drift: "); DEBUG_PRINTLN(rtcDrift);
            DEBUG_PRINT("Info: RTC time synced to "); printDateTime();
            blinkLed(5, 1000);
          }
        }
      }
      if (!rtcSyncFlag){
        DEBUG_PRINTLN("Warning: Unable to sync RTC!");
        blinkLed(10, 500);
      }
    }
    else {
      DEBUG_PRINTLN("Warning: microSD or GNSS offline!");
      rtcSyncFlag = false;
    }
}

// Print the RTC's date and time
void printDateTime() {
  if (logMode == 3 || logMode == 2){
    DEBUG_PRINTLN("Continuous");
  } else {
    rtc.getTime(); // Get the RTC's date and time
    char dateTimeBuffer[25];
    sprintf(dateTimeBuffer, "20%02d-%02d-%02d %02d:%02d:%02d",
            rtc.year, rtc.month, rtc.dayOfMonth,
            rtc.hour, rtc.minute, rtc.seconds, rtc.hundredths);
    DEBUG_PRINTLN(dateTimeBuffer);
  }
}

void printAlarm() {
  rtc.getAlarm(); // Get the RTC's alarm date and time
  char alarmBuffer[25];
  sprintf(alarmBuffer, "20%02d-%02d-%02d %02d:%02d:%02d",
          rtc.year, rtc.alarmMonth, rtc.alarmDayOfMonth,
          rtc.alarmHour, rtc.alarmMinute, rtc.alarmSeconds, rtc.alarmHundredths);
  DEBUG_PRINTLN(alarmBuffer);
}
    
 
