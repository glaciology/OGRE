void configureSD() {
  ///////// SD CARD INITIALIZATION
  if (!sd.begin(SD_CONFIG)) { // 
    DEBUG_PRINTLN("Warning: Card failed, or not present. Trying again...");
    delay(2000);

    if (!sd.begin(SD_CONFIG)) { // 
      DEBUG_PRINTLN("Warning: Card failed, or not present. Restarting...");
      online.uSD = false;
      peripheralPowerOff();
      while (1) {
        blinkLed(2, 250);
        delay(2000);
      }
    }
    else {
      DEBUG_PRINTLN("Info: microSD initialized.");
      online.uSD = true;
    }
  }
  else {
    DEBUG_PRINTLN("Info: microSD initialized.");
    online.uSD = true;
  }
}

// Create timestamped log file name
void getLogFileName() { 
  rtc.getTime();
  sprintf(logFileNameDate, "%04d_20%02d%02d%02d_%02d%02d.ubx", stationName,
          rtc.year, rtc.month, rtc.dayOfMonth,
          rtc.hour, rtc.minute);
}


void getConfig() {
  ///////// gets configuration settings from user via SD card
  int n;
  int i = 0;
  
  // OPEN CONFIG file
  configFile.open("CONFIG.TXT", O_READ);
  
  // IF SD ERROR, ABORT getConfig
  if (!configFile.isOpen()) {
    DEBUG_PRINTLN("Warning: Could not open CONFIG.TXT");
    DEBUG_PRINTLN("Warning: Using hard-coded settings");
    blinkLed(6, 100);
    delay(1000);
    return;
  }

  while ((n = configFile.fgets(line, sizeof(line))) > 0) {

    if (line[n - 1] == '\n') {
      line[n-1] = 0;
    }
    
    char* parse1 = strtok(line, "=");               // split at '='

    if (strcmp(parse1, "BAT_SHUTDOWN_V") == 0) {      // this value is a float
      float hold = strtof(strtok(NULL, "="), NULL);
      shutdownThreshold = hold;
    } else {                                          // the remaining values are all strings
      int hold = strtol(strtok(NULL, "="), NULL, 10); // take remaining string, convert to base 10
      settings[i] = hold;
    }
    i++;
  }
  
  logMode         = settings[0];
  logStartHr      = settings[1];
  logEndHr        = settings[2];
  logStartDay     = settings[3];
  epochSleep      = settings[4];
  ledBlink        = (settings[5] != 0);
  measureBattery  = (settings[6] != 0);
  logGPS          = settings[7];
  logGLO          = settings[8];
  logGAL          = settings[9];
  logBDS          = settings[10];
  logQZSS         = settings[11];
  logNav          = settings[12];
  stationName     = settings[13];
  measurementRate = settings[14];
  winterInterval  = settings[16];
  startMonth      = settings[17];
  endMonth        = settings[18];
  startDay        = settings[19];
  endDay          = settings[20];

  DEBUG_PRINT("Info: SD Settings from Device #: "); DEBUG_PRINTLN(stationName);
  if (logMode == 1 ) {
    DEBUG_PRINT("Log Mode 1 - Start/End Hours: "); DEBUG_PRINT(logStartHr); DEBUG_PRINT(", ");DEBUG_PRINTLN(logEndHr);
  }
  
  if (logMode == 3 ) {
    DEBUG_PRINT("Log Mode 3 - Day of Month to Log: "); DEBUG_PRINTLN(logStartDay);
  }

  if (logMode == 4 ) {
    DEBUG_PRINT("Log Mode 4 - Sleep Interval "); DEBUG_PRINTLN(epochSleep);
  }

  if (logMode == 6 ) {
    DEBUG_PRINT("Log Mode 6 - Winter Interval "); DEBUG_PRINT(winterInterval); DEBUG_PRINT("Summer months: "); DEBUG_PRINT(startMonth); DEBUG_PRINT(endMonth);
  } else { 
    DEBUG_PRINT(" - Log Mode: "); DEBUG_PRINTLN(logMode);
  }
  
  DEBUG_PRINT(" - Log Rate: "); DEBUG_PRINTLN(measurementRate);
  DEBUG_PRINT(" - Log Battery?: "); DEBUG_PRINT(measureBattery); DEBUG_PRINT(" Shutdown V: "); DEBUG_PRINTLN(shutdownThreshold);
  DEBUG_PRINT(" - Flash LED?: "); DEBUG_PRINTLN(ledBlink);
  DEBUG_PRINT(" - Constellations: "); DEBUG_PRINT("GPS "); DEBUG_PRINT(logGPS); DEBUG_PRINT(" GLO "); DEBUG_PRINT(logGLO);
  DEBUG_PRINT(" GAL "); DEBUG_PRINTLN(logGAL); DEBUG_PRINT("                   BDS "); DEBUG_PRINT(logBDS); DEBUG_PRINT(" QZSS "); 
  DEBUG_PRINT(logQZSS); DEBUG_PRINT(" NAV "); DEBUG_PRINTLN(logNav);
  
  if (!configFile.close()) {
    DEBUG_PRINTLN("Warning: Failed to close config file.");
    closeFailCounter++; // Count number of failed file closes
    while(1) {
    }
  }
}


void getDates() {
  ///////// Gets Dates in Unix Epoch format from user via SD if using log mode 5
  if (logMode == 5) {
    int n;
    int i = 0;
    
    // OPEN CONFIG file
    dateFile.open("EPOCH.TXT", O_READ);
    
    // IF SD ERROR, ABORT getConfig
    if (!dateFile.isOpen()){
      DEBUG_PRINTLN("Warning: Could not open EPOCH.TXT");
      DEBUG_PRINTLN("Warning: Using hard-coded sleep duration");
      blinkLed(5, 100);
      delay(1000);
      return;
    }
  
    while ((n = dateFile.fgets(line, sizeof(line))) > 0) {
  
      if (line[n - 1] == '\n') { // strip end line character
        line[n-1] = 0;
      }
      
      char* parse1 = strtok(line, "=");               // split at '='
      int hold = strtol(strtok(NULL, "="), NULL, 10); // take remaining string, convert to base 10
      dates[i] = hold;
      i++;
    }
    
//    for(int i=0; i<20; i++) {
//      DEBUG_PRINT(dates[i]); DEBUG_PRINTLN(" ");
//    }
    
    if (!dateFile.close()) {
      DEBUG_PRINTLN("Warning: Failed to close dates file.");
      closeFailCounter++; // Count number of failed file closes
      while(1) {
      }
    }
  }
}
