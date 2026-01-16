/* the WDT and RTC Log alarm functions are adapted from Sparkfun examples Example2_WDT_LowPower
    and Example6_LowPower_Alarm.
    Sync RTC functionality (sync, getDateTime, printAlarm) from A. Garbo GVT.
    See license and readme.
*/

void configureWdt() {
  wdt.configure(WDT_1HZ, 12, 24); // 12 second interrupts, 24 second reset period

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
  */

  am_hal_rtc_int_clear(AM_HAL_RTC_INT_ALM); // Clear the RTC alarm interrupt

  // 1 = daily during defined hours, 2 = continuous (new file generated each midnight),
  // 3 = monthly on defined day, 4 = 24 hr log with defined spacing, 5 = programmed dates,
  // 6 = summer log continuously, winter during defined interval, 99 = test  mode

  if (logMode == 1) {
    rtc.setAlarm(logEndHr, 0, 0, 0, 0, 0);
    rtc.setAlarmMode(4); // match every day during logEndHr
  }

  else if (logMode == 2) { // continuous: will generate new file at midnight

    delayForRtcDrift(rtcDrift);

    rtc.setAlarm(0, 0, 0, 0, 0, 0);
    rtc.setAlarmMode(4); // match every day at midnight
  }

  else if (logMode == 3 || logMode == 4 || logMode == 5) {
    rtc.setAlarm(rtc.hour, rtc.minute, rtc.seconds, 0, 0, 0);
    rtc.setAlarmMode(4); // WILL LOG FOR 24 HOURS from power-on
  }

  else if (logMode == 6 ) {
    // WILL LOG until midnight, UTC during summer, or 24 Hours from power-on in winter

    int whichMonth = rtc.month;
    int whichDay = rtc.dayOfMonth;

    // First check if month falls into summer interval
    summerInterval = (startMonth <= endMonth && whichMonth >= startMonth && whichMonth <= endMonth) || // E.g., log June-August (northern hemisphere)
                     (startMonth > endMonth && (whichMonth >= startMonth || whichMonth <= endMonth));  // E.g., Dec-Feb (southern hemisphere)

    // Now, confirm month + day are inside summer interval
    if (summerInterval) {
      summerInterval = (whichMonth != startMonth || whichDay >= startDay) &&  
                       (whichMonth != endMonth || whichDay <= endDay);
    }

    if (summerInterval) { // log continously
      delayForRtcDrift(rtcDrift); // prevents a second file for the same day if rtc drift is fast
      DEBUG_PRINTLN("Info: Logging to Midnight (SUMMER)");
      rtc.setAlarm(0, 0, 0, 0, 0, 0);
      rtc.setAlarmMode(4);
    }

    else { // WINTER
      // check if intended start time is stale, due to battery or unitialized
      uint32_t nowEpoch = rtc.getEpoch();
      const uint32_t STALE_THRESHOLD = 300;  // 5 minutes in seconds

      if (intendedWakeEpoch == 0 || intendedWakeEpoch + STALE_THRESHOLD < nowEpoch) {
        intendedWakeEpoch = nowEpoch;
        DEBUG_PRINT("Info: Stale intendedWakeEpoch, resetting to current epoch: ");
        DEBUG_PRINTLN(intendedWakeEpoch);
      }

      DEBUG_PRINTLN("Info: Logging 24 hours (WINTER MODE)");
      time_t a = intendedWakeEpoch + 86400UL; // 24 hours
      struct tm t;
      gmtime_r(&a, &t);
      rtc.setAlarm(t.tm_hour, t.tm_min, t.tm_sec, 0, t.tm_mday, t.tm_mon + 1);
      rtc.setAlarmMode(1); // Set the RTC alarm to match on exact date
    }
  }

  else if (logMode == 99) {
    time_t a = rtc.getEpoch() + secondsLog;
    struct tm t;
    gmtime_r(&a, &t);
    rtc.setAlarm(t.tm_hour, t.tm_min, t.tm_sec, 0, t.tm_mday, t.tm_mon + 1);
    rtc.setAlarmMode(1); // Set the RTC alarm to match on exact date
  }

  else if (logMode == 7) {
    // WILL LOG until midnight, UTC during summer, or 24 Hours from power-on in winter

    int whichMonth = rtc.hour;
    int whichDay = rtc.minute;

    // First check if month falls into summer interval
    summerInterval = (startMonth <= endMonth && whichMonth >= startMonth && whichMonth <= endMonth) ||
                     (startMonth > endMonth && (whichMonth >= startMonth || whichMonth <= endMonth));

    // Now, confirm month + day are inside summer interval
    if (summerInterval) {
      summerInterval = (whichMonth != startMonth || whichDay >= startDay) &&
                       (whichMonth != endMonth || whichDay <= endDay);
    }

    if (summerInterval) { // log continously
      delayForRtcDrift(rtcDrift); // prevents a second file for the same day if rtc drift is fast
      DEBUG_PRINTLN("Info: Logging to Midnight (SUMMER)");
      rtc.setAlarm(0, 0, 0, 0, 0, 0);
      rtc.setAlarmMode(6); // log until minute match
    }

    else { // WINTER
      // check if intended start time is stale, due to battery or unitialized
      uint32_t nowEpoch = rtc.getEpoch();
      const uint32_t STALE_THRESHOLD = 60UL/2;  // 30 seconds

      if (intendedWakeEpoch == 0 || intendedWakeEpoch + STALE_THRESHOLD < nowEpoch) {
        intendedWakeEpoch = nowEpoch;
        DEBUG_PRINT("Info: Stale intendedWakeEpoch, resetting to current epoch: ");
        DEBUG_PRINTLN(intendedWakeEpoch);
      }

      DEBUG_PRINTLN("Info: Logging 1 min (WINTER MODE)");
      time_t a = intendedWakeEpoch + 60UL; // 1 min
      struct tm t;
      gmtime_r(&a, &t); 
      rtc.setAlarm(t.tm_hour, t.tm_min, t.tm_sec, 0, t.tm_mday, t.tm_mon + 1);
      rtc.setAlarmMode(1); // Set the RTC alarm to match on exact date
    }
  }

  rtc.attachInterrupt();
  alarmFlag = false;
  DEBUG_PRINT("Info: Logging until "); printAlarm();
}


void configureSleepAlarm() {

  // Clear the RTC alarm interrupt
  am_hal_rtc_int_clear(AM_HAL_RTC_INT_ALM);

  if (logMode == 1) {
    rtc.setAlarm(logStartHr, 0, 0, 0, 0, 0); // (HR, MIN, SEC, HUND, DAY, MON) NO YR
    rtc.setAlarmMode(4); // match every day
  }

  else if (logMode == 2) {
    DEBUG_PRINTLN("Info: Log Mode 2. No sleep.");
    return;
  }

  else if (logMode == 3) {
    rtc.setAlarm(0, 0, 0, 0, logStartDay, 0);
    rtc.setAlarmMode(2); // match every month
  }

  else if (logMode == 4 || logMode == 5) {
    // sleeps until specified unix epoch; if no longer specified defaults to epochSleep interval
    // note that a maximum of 15 dates can be provided.
    time_t a;
    time_t b;
    for (int i = 0; i < sizeof(dates) / sizeof(dates[0]); i++) {
      if (dates[i] > rtc.getEpoch()) {
        DEBUG_PRINT("Info: Next date found: "); DEBUG_PRINTLN(dates[i]);
        b = dates[i];
        rtc.setAlarm(gmtime(&b)->tm_hour, gmtime(&b)->tm_min, gmtime(&b)->tm_sec, 0, gmtime(&b)->tm_mday, gmtime(&b)->tm_mon + 1);
        rtc.setAlarmMode(1); // Set the RTC alarm to match on minutes rollover
        rtc.attachInterrupt(); // Attach RTC alarm interrupt
        alarmFlag = false;
        DEBUG_PRINT("Info: Sleeping until: "); printAlarm();
        return;
      }
    }
    a = rtc.getEpoch() + epochSleep;
    rtc.setAlarm(gmtime(&a)->tm_hour, gmtime(&a)->tm_min, gmtime(&a)->tm_sec, 0, gmtime(&a)->tm_mday, gmtime(&a)->tm_mon + 1);
    rtc.setAlarmMode(1); // Set the RTC alarm to match on exact date
  }

  else if (logMode == 6 || logMode == 7) {
    time_t summerStartEpoch;
    time_t desiredWake;
    int whichMonth; 
    int whichDay; 
    
    if (logMode == 6) {
      whichMonth = rtc.month;
      whichDay = rtc.dayOfMonth;
    } else { 
      whichMonth = rtc.hour;
      whichDay = rtc.minute;
    }
    
    // First check if month falls into summer interval
    summerInterval = (startMonth <= endMonth && whichMonth >= startMonth && whichMonth <= endMonth) ||
                     (startMonth > endMonth && (whichMonth >= startMonth || whichMonth <= endMonth));

    // Now, confirm month + day are inside summer interval
    if (summerInterval) {
      summerInterval = (whichMonth != startMonth || whichDay >= startDay) &&
                       (whichMonth != endMonth || whichDay <= endDay);
    }

    if (summerInterval) {
      DEBUG_PRINTLN("Info: SUMMER MODE");
      return;
    } else {
      DEBUG_PRINTLN("Info: WINTER MODE");

      // FIGURE OUT IF SUMMER IS COMING SOONER THAN EXPECTED
      if (logMode == 6) {
        struct tm summerStartTm = {0};
        summerStartTm.tm_year = rtc.year + 100;       // current year (years since 1900)
        summerStartTm.tm_mon  = startMonth - 1;       // 0-11
        summerStartTm.tm_mday = startDay;             // 1-31
        summerStartTm.tm_hour = 0;
        summerStartTm.tm_min  = 0;
        summerStartTm.tm_sec  = 0;

        summerStartEpoch = mktime(&summerStartTm);

        // If summer start has already passed this year, use next year
        if (summerStartEpoch <= rtc.getEpoch()) {
            summerStartTm.tm_year += 1;
            summerStartEpoch = mktime(&summerStartTm);
        }
      
      } else { // LOG MODE 7
        struct tm summerStartTm = {0};
        summerStartTm.tm_year = rtc.year + 100;       // current year (years since 1900)
        summerStartTm.tm_mon  = rtc.month - 1;       // 0-11
        summerStartTm.tm_mday = rtc.dayOfMonth;             // 1-31
        summerStartTm.tm_hour = startMonth;
        summerStartTm.tm_min  = startDay;
        summerStartTm.tm_sec  = 0;

        summerStartEpoch = mktime(&summerStartTm);

        // If summer start has already passed this year, use next year
        if (summerStartEpoch <= rtc.getEpoch()) {
            summerStartTm.tm_mday += 1;
            summerStartEpoch = mktime(&summerStartTm);
        }
      }

      time_t winterWake = rtc.getEpoch() + winterInterval;
      desiredWake = (winterWake < summerStartEpoch) ? winterWake : summerStartEpoch;

      intendedWakeEpoch = (uint32_t)desiredWake;
      
      struct tm t;
      gmtime_r(&desiredWake, &t);
      rtc.setAlarm(t.tm_hour, t.tm_min, t.tm_sec, 0, t.tm_mday, t.tm_mon + 1);
      rtc.setAlarmMode(1); // Set the RTC alarm to match on exact date
    }
  }

  else if (logMode == 99) {
    time_t a = rtc.getEpoch() + secondsSleep;
    struct tm t;
    gmtime_r(&a, &t);
    rtc.setAlarm(t.tm_hour, t.tm_min, t.tm_sec, 0, t.tm_mday, t.tm_mon + 1);
    rtc.setAlarmMode(1); // Set the RTC alarm to match on exact date
  }

  rtc.attachInterrupt();
  alarmFlag = false;
  DEBUG_PRINT("Info: Sleeping until: "); printAlarm();
}


void syncRtc() {
  unsigned long loopStartTime = millis();
  online.rtcSync = false;

  DEBUG_PRINTLN("Info: Attempting to synchronize RTC with GNSS...");

  // Attempt to acquire a valid GNSS position fix for up to 3 minutes
  while (!online.rtcSync && millis() - loopStartTime < 3 * 60UL * 1000UL) {
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
        online.rtcSync = true;                         // Set flag, end SYNC
        gnss.setAutoPVTrate(0);                         // Turn off PVT rate

        DEBUG_PRINT("Info: RTC drift: "); DEBUG_PRINTLN(rtcDrift);
        DEBUG_PRINT("Info: RTC time synced to "); printDateTime();
      }
    }
  }
  if (!online.rtcSync) {
    DEBUG_PRINTLN("Warning: Unable to sync RTC! Awaiting System Reset");
    logDebug("RTC_SYNC");

    while (1) { //Awaiting WDT Reset
      blinkLed(5, 500);
      delay(2000);
    }
  }
}

// Print the RTC's date and time
void printDateTime() {
  rtc.getTime(); // Get the RTC's date and time
  char dateTimeBuffer[25];
  sprintf(dateTimeBuffer, "20%02d-%02d-%02d %02d:%02d:%02d",
          rtc.year, rtc.month, rtc.dayOfMonth,
          rtc.hour, rtc.minute, rtc.seconds);
  DEBUG_PRINTLN(dateTimeBuffer);
}

void printAlarm() {
  rtc.getAlarm(); // Get the RTC's alarm date and time
  char alarmBuffer[25];
  sprintf(alarmBuffer, "20%02d-%02d-%02d %02d:%02d:%02d",
          rtc.year, rtc.alarmMonth, rtc.alarmDayOfMonth,
          rtc.alarmHour, rtc.alarmMinute, rtc.alarmSeconds);
  DEBUG_PRINTLN(alarmBuffer);
}

// IF RTC is FAST (log mode 2, 6, 7)
void delayForRtcDrift(int rtcDriftSeconds) {
  // Only act when RTC fast
  if (rtcDriftSeconds >= 0 || rtcDriftSeconds < -600) return;

  // Total wait in milliseconds
  unsigned long remainingMs = (unsigned long)(-rtcDriftSeconds) * 1000UL;

  DEBUG_PRINT("RTC fast by ");
  DEBUG_PRINT(-rtcDriftSeconds);
  DEBUG_PRINT(" s — waiting ");
  DEBUG_PRINT(remainingMs);
  DEBUG_PRINTLN(" ms for true midnight...");

  // Break the wait into slices shorter than WDT timeout
  const unsigned long SLICE_MS = 1000;  // safe margin (WDT = 12s)

  while (remainingMs > 0) {
    // Compute slice
    unsigned long thisSlice = (remainingMs > SLICE_MS) ? SLICE_MS : remainingMs;

    delay(thisSlice);
    petDog();              // Keep watchdog happy

    remainingMs -= thisSlice;
  }
}
