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
  sprintf(logFileNameDate, "%s_20%02d%02d%02d_%02d%02d.ubx", stationName,
          rtc.year, rtc.month, rtc.dayOfMonth,
          rtc.hour, rtc.minute);
}


void getConfig() {
  ///////// gets configuration settings from user via SD card
  int n;                                                      // number of lines
  int i = 0;
  bool configError = false;
  
  // OPEN CONFIG file
  configFile.open("CONFIG.TXT", O_READ);
  
  // IF SD ERROR, ABORT getConfig
  if (!configFile.isOpen()) {
    DEBUG_PRINTLN("Warning: Could not open CONFIG.TXT");
    DEBUG_PRINTLN("Warning: Using hard-coded settings");
    createDefaultConfig();  // Generate default CONFIG.TXT
    blinkLed(6, 100);
    delay(1000);
    return;
  }

  // Count the number of lines in the file to validate
  int lineCount = 0;
  while ((n = configFile.fgets(line, sizeof(line))) > 0) {
    lineCount++;
  }

  // Verify file version has correct number of lines
  if (lineCount != CONFIG_FILE) {
    DEBUG_PRINT("Warning: detected an incompatable Config file. Your version: v");
    DEBUG_PRINT(lineCount); DEBUG_PRINT(" Should be: v");
    DEBUG_PRINTLN(CONFIG_FILE); DEBUG_PRINTLN("Warning: Using hard-coded settings");
    logDebug("CFG_FILE_VERSION");
    blinkLed(6, 100);
    delay(1000);

    if (!configFile.close()) {
      DEBUG_PRINTLN("Warning: Failed to close config file.");
      closeFailCounter++; // Count number of failed file closes
      while(1) {
      }
    }
    return;
  }
  
  configFile.seek(0);  // Reset file pointer to the beginning of the file

  // MAIN PARSING LOOP
  while ((n = configFile.fgets(line, sizeof(line))) > 0) {

    // Strip CR/LF *carriage return - 0x0D or line feed 0x0A (Mac) or CRLF (Windows)
    size_t len = strcspn(line, "\r\n");       // strcspn: length of line, excluding \r and \n
    line[len] = '\0';                         //  Strip \r and/or \n chars and null-terminate the string at the position of the first occurrence

    // Parse left side of "="
    char* key = strtok(line, "=");                              // split at '='
    if (!key) {
      configError = true;
      continue;
    }

    // Parse right side of "="
    if (strcmp(key, "BAT_SHUTDOWN_V(00.0, volts)") == 0) {      // shutdownThreshold is a float
      float hold = strtof(strtok(NULL, "="), NULL);
      shutdownThreshold = hold;
    } else if (strcmp(key, "STATION_NAME(0000, char)") == 0) {  // stationValue is a string
        char* stationValue = strtok(NULL, "=");  
        if (stationValue != NULL) {
            strncpy(stationName, stationValue, 4);
            stationName[4] = '\0';                                // Ensure null-termination
        }
    } else {                                                      // the remaining values are all strings
      int hold = strtol(strtok(NULL, "="), NULL, 10);             // take remaining string, convert to base 10
      settings[i] = hold;
    }
    i++;
  }
  

  //ASSIGN VALUES and CONDUCT CHECKS
  // Check if logMode is a valid log mode
  int logModeRead = settings[0];
  if (logModeRead == 1 || logModeRead == 2 || logModeRead == 3 || 
      logModeRead == 4 || logModeRead == 5 || logModeRead == 6 || 
      logModeRead == 7 || logModeRead == 99) {
      
      logMode = logModeRead;  // Set the logMode if it's valid
      
  } else {
      DEBUG_PRINTLN("Error: Invalid LOG_MODE value in CONFIG.TXT! Using default value.");
      configError = true;
  }

  logStartHr      = settings[1]; //no check
  logEndHr        = settings[2]; //no check
  logStartDay     = settings[3]; //no check
  epochSleep      = settings[4]; //no check

  int ledBlinkRead        = settings[5];
  int measureBatteryRead  = settings[6];
  if (ledBlinkRead == 0 || ledBlinkRead == 1) {
      ledBlink = (settings[5] != 0);
  } else {
      DEBUG_PRINTLN("Warning: Invalid value for LED_INDICATORS. Retaining default.");
      configError = true;
  }
  
  if (measureBatteryRead == 0 || measureBatteryRead == 1) {
      measureBattery = (settings[6] != 0);
  } else {
      DEBUG_PRINTLN("Warning: Invalid value for MEASURE_BATTERY. Retaining default.");
      configError = true;
  }
  
  configError |= checkAndAssign(logGPS, settings[7], "ENABLE_GPS");
  configError |= checkAndAssign(logGLO, settings[8], "ENABLE_GLO");
  configError |= checkAndAssign(logGAL, settings[9], "ENABLE_GAL");
  configError |= checkAndAssign(logBDS, settings[10], "ENABLE_BDS");
  configError |= checkAndAssign(logQZSS, settings[11], "ENABLE_QZSS");
  configError |= checkAndAssign(logNav, settings[12], "ENABLE_NAV_SFRBX");

  int measurementRateRead = settings[14];
  if (measurementRateRead >= 1 && measurementRateRead <=60) {
    measurementRate = settings[14];
  } else {
    DEBUG_PRINTLN("Warning: Invalid value for MEASUREMENT_RATE. Retaining default.");
    configError = true;
  }
  
  // Validate winterInterval
  int winterIntervalRead = settings[16];
  if (winterIntervalRead > 0) {
      winterInterval = winterIntervalRead;
  } else {
      DEBUG_PRINTLN("Warning: Invalid WINTER_INTERVAL. Retaining default.");
      configError = true;
  }
  
  // Validate startMonth
  int startMonthRead = settings[17];
  if (startMonthRead >= 1 && startMonthRead <= 12) {
      startMonth = startMonthRead;
  } else {
      DEBUG_PRINTLN("Warning: Invalid START_MONTH. Retaining default.");
      configError = true;
  }
  
  // Validate endMonth
  int endMonthRead = settings[18];
  if (endMonthRead >= 1 && endMonthRead <= 12) {
      endMonth = endMonthRead;
  } else {
      DEBUG_PRINTLN("Warning: Invalid END_MONTH. Retaining default.");
      configError = true;
  }
  
  // Validate startDay (cap at 28)
  int startDayRead = settings[19];
  if (startDayRead >= 1) {
      startDay = (startDayRead > 28) ? 28 : startDayRead;
  } else {
      DEBUG_PRINTLN("Warning: Invalid START_DAY. Retaining default.");
      configError = true;
  }
  
  // Validate endDay (cap at 28)
  int endDayRead = settings[20];
  if (endDayRead >= 1) {
      endDay = (endDayRead > 28) ? 28 : endDayRead;
  } else {
      DEBUG_PRINTLN("Warning: Invalid END_DAY. Retaining default.");
      configError = true;
  }

  // Validate TERMINAL_LOG
  int terminalLogRead = settings[21];
  if (terminalLogRead == 0 || terminalLogRead == 1) {
      terminalLogging = (settings[21] != 0);
  } else {
      DEBUG_PRINTLN("Warning: Invalid value for MEASURE_BATTERY. Retaining default.");
      configError = true;
  }
  
  if (terminalLogging && measurementRate < 5) {
    DEBUG_PRINTLN("Warning: Terminal logging disabled because measurementRate < 5s");
    terminalLogging = false;
  }

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
    DEBUG_PRINT("Log Mode 6 - Winter Interval "); DEBUG_PRINT(winterInterval); DEBUG_PRINT(" Summer months: "); DEBUG_PRINT(startMonth); DEBUG_PRINT(", "); DEBUG_PRINT(endMonth);
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

  if (configError) {
    DEBUG_PRINTLN("Warning: Invalid Config file value");
    logDebug("CFG_FILE_ERROR");
  }
  
}

bool checkAndAssign(int& variable, int readValue, const char* varName) {
    if (readValue == 0 || readValue == 1) {
        variable = readValue;
        return false;
    } else {
        DEBUG_PRINT("Warning: Invalid value for ");
        DEBUG_PRINT(varName);
        DEBUG_PRINTLN(". Retaining default.");
        return true;
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

    // Count the number of lines in the file
    int lineCount = 0;
    while ((n = dateFile.fgets(line, sizeof(line))) > 0) {
      lineCount++;
    }

    if (lineCount != EPOCH_FILE) {
      DEBUG_PRINT("Warning: detected an incompatable Config file. Your version: v");
      DEBUG_PRINT(lineCount); DEBUG_PRINT(" Should be: v");
      DEBUG_PRINTLN(CONFIG_FILE); DEBUG_PRINTLN("Warning: Using hard-coded settings");
      blinkLed(5, 100);
      delay(1000);
      return;
    }
    
    dateFile.seek(0);  // Reset file pointer to the beginning of the file
  
    while ((n = dateFile.fgets(line, sizeof(line))) > 0) {

      size_t len = strcspn(line, "\r\n");
      line[len] = '\0';  //  Strip \r and/or \n chars and null-terminate the string at the position of the first occurrence
      
      char* key = strtok(line, "=");               // split at '='
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

void createDefaultConfig() {
    if (!configFile.open("CONFIG.TXT", O_CREAT | O_WRITE)) { // Create file if new, Append to end, Open to Write
      DEBUG_PRINTLN("Warning: Failed to create CONFIG file.");
      return;
    } else {
      DEBUG_PRINTLN("Info: Created config file"); 
    }

    // Write default configuration settings to the file
    configFile.print("LOG_MODE(1: daily hr, 2: cont, 3: mon, 4: 24 roll, 5: date, 6: season, 99: test)=");
    configFile.println(logMode);
    configFile.print("LOG_START_HOUR_UTC(mode 1 only)=");
    configFile.println(logStartHr);
    configFile.print("LOG_END_HOUR_UTC(mode 1 only)=");
    configFile.println(logEndHr);
    configFile.print("LOG_START_DAY(mode 3 only, 0-28)=");
    configFile.println(logStartDay);
    configFile.print("LOG_EPOCH_SLEEP(modes 4/5 only, seconds)=");
    configFile.println(epochSleep);
    configFile.print("LED_INDICATORS(0-false, 1-true)=");
    configFile.println(ledBlink ? "1" : "0");  // Ternary operator!
    configFile.print("MEASURE_BATTERY(0-false, 1-true)=");
    configFile.println(measureBattery ? "1" : "0"); 
    configFile.print("ENABLE_GPS(0-false, 1-true)=");
    configFile.println(logGPS ? "1" : "0");
    configFile.print("ENABLE_GLO(0-false, 1-true)=");
    configFile.println(logGLO ? "1" : "0");
    configFile.print("ENABLE_GAL(0-false, 1-true)=");
    configFile.println(logGAL ? "1" : "0");
    configFile.print("ENABLE_BDS(0-false, 1-true)=");
    configFile.println(logBDS ? "1" : "0");
    configFile.print("ENABLE_QZSS(0-false, 1-true)=");
    configFile.println(logQZSS ? "1" : "0");
    configFile.print("ENABLE_NAV_SFRBX(0-false, 1-true)=");
    configFile.println(logNav ? "1" : "0");
    configFile.print("STATION_NAME(0000, char)=");
    configFile.println(stationName);
    configFile.print("MEASURE_RATE(integer seconds)=");
    configFile.println(measurementRate);
    configFile.print("BAT_SHUTDOWN_V(00.0, volts)=");
    configFile.println(shutdownThreshold);
    configFile.print("WINTER_INTERVAL(seconds, mode 6 only)=");
    configFile.println(winterInterval);
    configFile.print("SUMMER_START_MONTH(mode 6 only)=");
    configFile.println(startMonth);
    configFile.print("SUMMER_END_MONTH(mode 6 only)=");
    configFile.println(endMonth);
    configFile.print("SUMMER_START_DAY(mode 6 only)=");
    configFile.println(startDay);
    configFile.print("SUMMER_END_DAY(mode 6 only)=");
    configFile.println(endDay);
    configFile.println(SOFTWARE_VERSION " end;"); // End marker for config

    // Sync the debug file
    if (!configFile.sync()) {
      DEBUG_PRINTLN("Warning: Failed to sync CONFIG file.");
    }
    // Close the file
    if (!configFile.close()) {
      DEBUG_PRINTLN("Warning: Failed to close CONFIG file.");
    }
}
