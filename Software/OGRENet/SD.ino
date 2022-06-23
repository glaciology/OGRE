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
    DEBUG_PRINTLN("Info: SD card initialized.");
    online.uSD = true;
  }
}

// Create timestamped log file name
void getLogFileName() { 
  rtc.getTime();
  sprintf(logFileNameDate, "%04d_20%02d%02d%02d_%02d%02d%02d.ubx", stationName,
          rtc.year, rtc.month, rtc.dayOfMonth,
          rtc.hour, rtc.minute, rtc.seconds);
}


void getConfig() {
  ///////// gets configuration settings from user via SD card
  int n;
  int i = 0;
  
  // OPEN CONFIG file
  configFile.open("CONFIG.TXT", O_READ);
  
  // IF SD ERROR, ABORT getConfig
  if (!configFile.isOpen()){
    DEBUG_PRINTLN("Warning: Could not open CONFIG.TXT");
    DEBUG_PRINTLN("Warning: Using hard-coded settings");
    blinkLed(5, 100);
    delay(1000);
    return;
  }

  while ((n = configFile.fgets(line, sizeof(line))) > 0) {

    if (line[n - 1] == '\n') {
      line[n-1] = 0;
    }
    
    char* parse1 = strtok(line, "=");               // split at '='
    int hold = strtol(strtok(NULL, "="), NULL, 10); // take remaining string, convert to base 10
    settings[i] = hold;
    i++;
  }
  
  logMode         = settings[0];
  logStartHr      = settings[1];
  logEndHr        = settings[2];
  logStartDay     = settings[3];
  epochSleep      = settings[4];

  if (settings[5] == 0){
    ledBlink = false;
  } else {
    ledBlink = true;
  }

  if (settings[6] == 0){
    measureBattery = false;
  } else  {
    measureBattery = true;
  }

  logGPS = settings[7];
  logGLO = settings[8];
  logGAL = settings[9];
  logBDS = settings[10];
  logQZSS = settings[11];
  logNav = settings[12];
  stationName = settings[13];
  
  DEBUG_PRINTLN("Info: Settings read from SD:");
  DEBUG_PRINT("1. Log Mode: "); DEBUG_PRINTLN(logMode);
  DEBUG_PRINT("2. Log Battery?: "); DEBUG_PRINTLN(measureBattery);
  DEBUG_PRINT("3. Flash LED?: "); DEBUG_PRINTLN(ledBlink);

  if (logMode == 1 ){
    DEBUG_PRINT("Log Mode 1 - Start/End Hours: "); DEBUG_PRINT(logStartHr); DEBUG_PRINT(", ");DEBUG_PRINTLN(logEndHr);
  }
  
  if (logMode == 3 ){
    DEBUG_PRINT("Log Mode 3 - Day of Month to Log: "); DEBUG_PRINTLN(logStartDay);
  }

  if (logMode == 4 ){
    DEBUG_PRINT("Log Mode 4 - Sleep Interval "); DEBUG_PRINTLN(epochSleep);
  }

  DEBUG_PRINT("4. Constellations: "); DEBUG_PRINT("GPS "); DEBUG_PRINT(logGPS); DEBUG_PRINT(" GLO "); DEBUG_PRINTLN(logGLO);
  DEBUG_PRINT(" GAL "); DEBUG_PRINT(logGAL); DEBUG_PRINT(" BDS "); DEBUG_PRINT(logBDS); DEBUG_PRINT(" QZSS "); DEBUG_PRINT(logQZSS);
  DEBUG_PRINT(" NAV "); DEBUG_PRINTLN(logNav);
  
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
  
      if (line[n - 1] == '\n') {
        line[n-1] = 0;
      }
      
      char* parse1 = strtok(line, "=");               // split at '='
      int hold = strtol(strtok(NULL, "="), NULL, 10); // take remaining string, convert to base 10
      dates[i] = hold;
      i++;
    }
    
//    for(int i=0; i<15; i++) {
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
