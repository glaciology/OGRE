void configureSD() {
  ///////// SD CARD INITIALIZATION
  if (!sd.begin(SD_CONFIG)) { // 
    DEBUG_PRINTLN("Warning: Card failed, or not present. Trying again...");
    delay(2000);

    if (!sd.begin(SD_CONFIG)) { // 
      DEBUG_PRINTLN("Warning: Card failed, or not present...");
      online.uSD = false;
      while (1) {
        blinkLed(2, 250);
        delay(2000);
      }
    }
    else{
      DEBUG_PRINTLN("Info: microSD initialized.");
      online.uSD = true;
    }
  }
  else{
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

  DEBUG_PRINTLN(logFileNameDate);
}


void getConfig() {
  
  int n;
  int i = 0;
  
  // OPEN CONFIG file
  configFile.open("CONFIG.TXT", O_READ);
  
  // IF SD ERROR, ABORT getConfig
  if (!configFile.isOpen()){
    DEBUG_PRINTLN("Warning: Could not open CONFIG.TXT");
    DEBUG_PRINTLN("Warning: Using hard-coded settings");
    blinkLed(5, 100);
    return;
  }

  while ((n = configFile.fgets(line, sizeof(line))) > 0) {
    if (line[n - 1] == '\n') {
      // remove '\n'
      line[n-1] = 0;
      char* parse1 = strtok(line, "=");
      int hold = strtol(strtok(NULL,"="), NULL, 10); //first split at '=', then take right-side data in base 10
      settings[i] = hold;
      i++;
      
    } else {
      // no '\n' - line too long or missing '\n' at EOF
      //DEBUG_PRINTLN("Warning: Line too long or missing '\n' at EOF: Check formatting");
    }
  }
  
  logMode     = settings[0];
  logStartHr  = settings[1];
  logEndHr    = settings[2];
  logStartDay = settings[3];

  if (settings[4] == 0){
    ledBlink = false;
  } else {
    ledBlink = true;
  }

  if (settings[5] == 0){
    measureBattery = false;
  } else  {
    measureBattery = true;
  }

  logGPS = settings[6];
  logGLO = settings[7];
  logGAL = settings[8];
  logBDS = settings[9];
  logQZSS = settings[10];
  logNav = settings[11];
  stationName = settings[12];
  
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

  DEBUG_PRINT("4. Constellations: "); DEBUG_PRINT("GPS "); DEBUG_PRINT(logGPS); DEBUG_PRINT(" GLO "); DEBUG_PRINTLN(logGLO);
  DEBUG_PRINT(" GAL "); DEBUG_PRINT(logGAL); DEBUG_PRINT(" BDS "); DEBUG_PRINT(logBDS); DEBUG_PRINT(" QZSS "); DEBUG_PRINT(logQZSS);
  DEBUG_PRINT(" NAV "); DEBUG_PRINTLN(logNav);
  
  if (!configFile.close()) {
    DEBUG_PRINTLN("Warning: Failed to close config file.");
    closeFailCounter++; // Count number of failed file closes
  }
}
