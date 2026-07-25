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
        blinkLed(2, 250, RED);
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

// Generic validated-assign helpers
bool assignBool(int& var, long v, const char* name) {
  if (v == 0 || v == 1) { var = (int)v; return true; }
  DEBUG_PRINT("Warning: Invalid value for "); DEBUG_PRINTLN(name);
  return false;
}

bool assignIntRange(int& var, long v, int lo, int hi, const char* name) {
  if (v >= lo && v <= hi) { var = (int)v; return true; }
  DEBUG_PRINT("Warning: Invalid value for "); DEBUG_PRINTLN(name);
  return false;
}

bool assignByteRange(byte& var, long v, int lo, int hi, const char* name) {
  if (v >= lo && v <= hi) { var = (byte)v; return true; }
  DEBUG_PRINT("Warning: Invalid value for "); DEBUG_PRINTLN(name);
  return false;
}

bool assignU32Range(uint32_t& var, long v, uint32_t lo, uint32_t hi, const char* name) {
  if (v < 0) {
    DEBUG_PRINT("Warning: Invalid value for "); DEBUG_PRINTLN(name);
    return false;
  }
  uint32_t uv = (uint32_t)v;
  if (uv >= lo && uv <= hi) { var = uv; return true; }
  DEBUG_PRINT("Warning: Invalid value for "); DEBUG_PRINTLN(name);
  return false;
}

bool assignFloatRange(float& var, float v, float lo, float hi, const char* name) {
  if (v >= lo && v <= hi) { var = v; return true; }
  DEBUG_PRINT("Warning: Invalid value for "); DEBUG_PRINTLN(name);
  return false;
}

// Key indices for the "found" bitmask
enum ConfigKeyBit {
  K_LOG_MODE, K_LOG_START_HR, K_LOG_END_HR, K_LOG_START_HR2, K_LOG_END_HR2,
  K_LOG_START_DAY, K_EPOCH_SLEEP, K_LED, K_MEAS_BAT,
  K_GPS, K_GLO, K_GAL, K_BDS, K_QZSS, K_NAV,
  K_STATION, K_MEAS_RATE, K_BAT_V, K_WINTER,
  K_SUM_START_MO, K_SUM_END_MO, K_SUM_START_DAY, K_SUM_END_DAY, K_TERM_LOG,
  K_COUNT
};

const char* CONFIG_KEY_NAMES[K_COUNT] = {
  "LOG_MODE","LOG_START_HOUR_UTC","LOG_END_HOUR_UTC","LOG_START_HOUR_TWO_UTC","LOG_END_HOUR_TWO_UTC",
  "LOG_START_DAY","LOG_EPOCH_SLEEP","LED_INDICATORS","MEASURE_BATTERY",
  "ENABLE_GPS","ENABLE_GLO","ENABLE_GAL","ENABLE_BDS","ENABLE_QZSS","ENABLE_NAV_SFRBX",
  "STATION_NAME","MEASURE_RATE","BAT_SHUTDOWN_V","WINTER_INTERVAL",
  "SUMMER_START_MONTH","SUMMER_END_MONTH","SUMMER_START_DAY","SUMMER_END_DAY","OUTPUT_UART_RAWX"
};

void getConfig() {
  bool configError = false;      // any value present but invalid
  uint32_t found = 0;            // bitmask of keys successfully parsed+assigned

  configFile.open("CONFIG.TXT", O_READ);
  if (!configFile.isOpen()) {
    DEBUG_PRINTLN("Warning: Could not open CONFIG.TXT");
    DEBUG_PRINTLN("Warning: Using hard-coded settings");
    createDefaultConfig();
    blinkLed(6, 100, RED);
    delay(1000);
    return;
  }

  int n;
  while ((n = configFile.fgets(line, sizeof(line))) > 0) {
    size_t len = strcspn(line, "\r\n");
    line[len] = '\0';
    if (len == 0) continue;                     // skip blank lines

    char* key = strtok(line, "=");
    char* valStr = key ? strtok(NULL, "=") : NULL;
    if (!key || !valStr) continue;               // no '=' -> not a setting line (e.g. version marker)

    // Strip parenthetical description so key text can change freely
    char* paren = strchr(key, '(');
    if (paren) *paren = '\0';

    bool matched = true;
    long v = strtol(valStr, NULL, 10);            // used for all int-typed keys

    if      (!strcmp(key, "LOG_MODE")) {
      if (v==1||v==2||v==3||v==4||v==5||v==6||v==7||v==8||v==99) { logMode = (byte)v; found |= 1UL<<K_LOG_MODE; }
      else { configError = true; DEBUG_PRINTLN("Warning: Invalid LOG_MODE."); }
    }
    else if (!strcmp(key, "LOG_START_HOUR_UTC"))     { if (assignByteRange(logStartHr, v, 0, 23, key)) found |= 1UL<<K_LOG_START_HR; else configError = true; }
    else if (!strcmp(key, "LOG_END_HOUR_UTC"))       { if (assignByteRange(logEndHr, v, 0, 23, key)) found |= 1UL<<K_LOG_END_HR; else configError = true; }
    else if (!strcmp(key, "LOG_START_HOUR_TWO_UTC")) { if (assignByteRange(logStartHrTWO, v, 0, 23, key)) found |= 1UL<<K_LOG_START_HR2; else configError = true; }
    else if (!strcmp(key, "LOG_END_HOUR_TWO_UTC"))   { if (assignByteRange(logEndHrTWO, v, 0, 23, key)) found |= 1UL<<K_LOG_END_HR2; else configError = true; }
    else if (!strcmp(key, "LOG_START_DAY"))          { if (assignByteRange(logStartDay, v, 1, 28, key)) found |= 1UL<<K_LOG_START_DAY; else configError = true; }
    else if (!strcmp(key, "LOG_EPOCH_SLEEP"))        { if (assignU32Range(epochSleep, v, 1, 4294967295UL, key)) found |= 1UL<<K_EPOCH_SLEEP; else configError = true; }
    else if (!strcmp(key, "LED_INDICATORS")) {
      int hold = ledBlink;
      if (assignBool(hold, v, key)) { ledBlink = (hold != 0); found |= 1UL<<K_LED; } else configError = true;
    }
    else if (!strcmp(key, "MEASURE_BATTERY")) {
      int hold = measureBattery;
      if (assignBool(hold, v, key)) { measureBattery = (hold != 0); found |= 1UL<<K_MEAS_BAT; } else configError = true;
    }
    else if (!strcmp(key, "ENABLE_GPS"))         { if (assignBool(logGPS, v, key)) found |= 1UL<<K_GPS; else configError = true; }
    else if (!strcmp(key, "ENABLE_GLO"))         { if (assignBool(logGLO, v, key)) found |= 1UL<<K_GLO; else configError = true; }
    else if (!strcmp(key, "ENABLE_GAL"))         { if (assignBool(logGAL, v, key)) found |= 1UL<<K_GAL; else configError = true; }
    else if (!strcmp(key, "ENABLE_BDS"))         { if (assignBool(logBDS, v, key)) found |= 1UL<<K_BDS; else configError = true; }
    else if (!strcmp(key, "ENABLE_QZSS"))        { if (assignBool(logQZSS, v, key)) found |= 1UL<<K_QZSS; else configError = true; }
    else if (!strcmp(key, "ENABLE_NAV_SFRBX"))   { if (assignBool(logNav, v, key)) found |= 1UL<<K_NAV; else configError = true; }
    else if (!strcmp(key, "STATION_NAME")) {
      if (strlen(valStr) > 0) {
        strncpy(stationName, valStr, 4);
        stationName[4] = '\0';
        found |= 1UL<<K_STATION;
      } else { configError = true; DEBUG_PRINTLN("Warning: Empty STATION_NAME."); }
    }
    else if (!strcmp(key, "MEASURE_RATE"))       { if (assignIntRange(measurementRate, v, 1, 60, key)) found |= 1UL<<K_MEAS_RATE; else configError = true; }
    else if (!strcmp(key, "BAT_SHUTDOWN_V")) {
      float fv = strtof(valStr, NULL);
      if (assignFloatRange(shutdownThreshold, fv, 5.0, 16.0, key)) found |= 1UL<<K_BAT_V; else configError = true;
    }
    else if (!strcmp(key, "WINTER_INTERVAL"))    { if (assignU32Range(winterInterval, v, 1, 4294967295UL, key)) found |= 1UL<<K_WINTER; else configError = true; }
    else if (!strcmp(key, "SUMMER_START_MONTH")) { if (assignByteRange(startMonth, v, 1, 12, key)) found |= 1UL<<K_SUM_START_MO; else configError = true; }
    else if (!strcmp(key, "SUMMER_END_MONTH"))   { if (assignByteRange(endMonth, v, 1, 12, key)) found |= 1UL<<K_SUM_END_MO; else configError = true; }
    else if (!strcmp(key, "SUMMER_START_DAY"))   { if (assignByteRange(startDay, v, 1, 28, key)) found |= 1UL<<K_SUM_START_DAY; else configError = true; }
    else if (!strcmp(key, "SUMMER_END_DAY"))     { if (assignByteRange(endDay, v, 1, 28, key)) found |= 1UL<<K_SUM_END_DAY; else configError = true; }
    else if (!strcmp(key, "OUTPUT_UART_RAWX")) {
      int hold = terminalLogging;
      if (assignBool(hold, v, key)) { terminalLogging = (hold != 0); found |= 1UL<<K_TERM_LOG; } else configError = true;
    }
    else { matched = false; }

    if (!matched) {
      DEBUG_PRINT("Warning: Unrecognized config key: "); DEBUG_PRINTLN(key);
      configError = true;
    }
  }

  if (!configFile.close()) {
    DEBUG_PRINTLN("Warning: Failed to close config file.");
    closeFailCounter++;
    while (1) {}
  }

  if (terminalLogging && measurementRate < 5) {
    DEBUG_PRINTLN("Warning: Terminal logging disabled because measurementRate < 5s");
    terminalLogging = false;
  }

  // Report any keys never found -> defaults were used for those
  bool anyMissing = false;
  for (int k = 0; k < K_COUNT; k++) {
    if (!(found & (1UL << k))) {
      if (!anyMissing) { DEBUG_PRINTLN("Warning: Using defaults for missing/invalid keys:"); anyMissing = true; }
      DEBUG_PRINT("  "); DEBUG_PRINTLN(CONFIG_KEY_NAMES[k]);
    }
  }

  DEBUG_PRINT("Info: SD Settings from Device #: "); DEBUG_PRINTLN(stationName);

  if (logMode == 1) {
    DEBUG_PRINT("Log Mode 1 - Start/End Hours: "); DEBUG_PRINT(logStartHr); DEBUG_PRINT(", "); DEBUG_PRINTLN(logEndHr);
  }
  if (logMode == 3) {
    DEBUG_PRINT("Log Mode 3 - Day of Month to Log: "); DEBUG_PRINTLN(logStartDay);
  }
  if (logMode == 4) {
    DEBUG_PRINT("Log Mode 4 - Sleep Interval: "); DEBUG_PRINTLN(epochSleep);
  }
  if (logMode == 6) {
    DEBUG_PRINT("Log Mode 6 - Winter Interval: "); DEBUG_PRINT(winterInterval);
    DEBUG_PRINT(" Summer months: "); DEBUG_PRINT(startMonth); DEBUG_PRINT(", "); DEBUG_PRINTLN(endMonth);
  }
  if (logMode == 8) {
    DEBUG_PRINT("Log Mode 8 - Session 1: "); DEBUG_PRINT(logStartHr); DEBUG_PRINT(", "); DEBUG_PRINT(logEndHr);
    DEBUG_PRINT(" Session 2: "); DEBUG_PRINT(logStartHrTWO); DEBUG_PRINT(", "); DEBUG_PRINTLN(logEndHrTWO);
  }

  DEBUG_PRINT(" - Log Mode: "); DEBUG_PRINTLN(logMode);
  DEBUG_PRINT(" - Log Rate: "); DEBUG_PRINTLN(measurementRate);
  DEBUG_PRINT(" - Log Battery?: "); DEBUG_PRINT(measureBattery); DEBUG_PRINT(" Shutdown V: "); DEBUG_PRINTLN(shutdownThreshold);
  DEBUG_PRINT(" - Flash LED?: "); DEBUG_PRINTLN(ledBlink);
  DEBUG_PRINT(" - Constellations: "); DEBUG_PRINT("GPS "); DEBUG_PRINT(logGPS);
  DEBUG_PRINT(" GLO "); DEBUG_PRINT(logGLO);
  DEBUG_PRINT(" GAL "); DEBUG_PRINTLN(logGAL);
  DEBUG_PRINT("                   BDS "); DEBUG_PRINT(logBDS);
  DEBUG_PRINT(" QZSS "); DEBUG_PRINT(logQZSS);
  DEBUG_PRINT(" NAV "); DEBUG_PRINTLN(logNav);

  if (configError || anyMissing) {
    DEBUG_PRINTLN("Warning: Config file incomplete or contains invalid value(s).");
    logDebug("CFG_FILE_ERROR");
  }
}

// void getConfig() {
//   ///////// gets configuration settings from user via SD card
//   int n;                                                      // number of lines
//   int i = 0;
//   bool configError = false;
  
//   // OPEN CONFIG file
//   configFile.open("CONFIG.TXT", O_READ);
  
//   // IF SD ERROR, ABORT getConfig
//   if (!configFile.isOpen()) {
//     DEBUG_PRINTLN("Warning: Could not open CONFIG.TXT");
//     DEBUG_PRINTLN("Warning: Using hard-coded settings");
//     createDefaultConfig();  // Generate default CONFIG.TXT
//     blinkLed(6, 100, RED);
//     delay(1000);
//     return;
//   }

//   // Count the number of lines in the file to validate
//   int lineCount = 0;
//   while ((n = configFile.fgets(line, sizeof(line))) > 0) {
//     lineCount++;
//   }

//   // Verify file version has correct number of lines
//   if (lineCount != CONFIG_FILE) {
//     DEBUG_PRINT("Warning: detected an incompatable Config file. Your version has this many settings:");
//     DEBUG_PRINT(lineCount); DEBUG_PRINT(" Should be: ");
//     DEBUG_PRINTLN(CONFIG_FILE); DEBUG_PRINTLN("Warning: Using hard-coded settings");
//     logDebug("CFG_FILE_VERSION");
//     blinkLed(6, 100, RED);
//     delay(1000);

//     if (!configFile.close()) {
//       DEBUG_PRINTLN("Warning: Failed to close config file.");
//       closeFailCounter++; // Count number of failed file closes
//       while(1) {
//       }
//     }
//     return;
//   }
  
//   configFile.seek(0);  // Reset file pointer to the beginning of the file

//   // MAIN PARSING LOOP
//   while ((n = configFile.fgets(line, sizeof(line))) > 0) {

//     // Strip CR/LF *carriage return - 0x0D or line feed 0x0A (Mac) or CRLF (Windows)
//     size_t len = strcspn(line, "\r\n");       // strcspn: length of line, excluding \r and \n
//     line[len] = '\0';                         //  Strip \r and/or \n chars and null-terminate the string at the position of the first occurrence

//     // Parse left side of "="
//     char* key = strtok(line, "=");                              // split at '='
//     if (!key) {
//       configError = true;
//       continue;
//     }

//     // Parse right side of "="
//     if (strcmp(key, "BAT_SHUTDOWN_V(00.0, volts)") == 0) {      // shutdownThreshold is a float
//       float hold = strtof(strtok(NULL, "="), NULL);
//       shutdownThreshold = hold;
//     } else if (strcmp(key, "STATION_NAME(0000, char)") == 0) {  // stationValue is a string
//         char* stationValue = strtok(NULL, "=");  
//         if (stationValue != NULL) {
//             strncpy(stationName, stationValue, 4);
//             stationName[4] = '\0';                                // Ensure null-termination
//         }
//     } else {                                                      // the remaining values are all strings
//       int hold = strtol(strtok(NULL, "="), NULL, 10);             // take remaining string, convert to base 10
//       settings[i] = hold;
//     }
//     i++;
//   }
  

//   //ASSIGN VALUES and CONDUCT CHECKS
//   // Check if logMode is a valid log mode
//   int logModeRead = settings[0];
//   if (logModeRead == 1 || logModeRead == 2 || logModeRead == 3 || 
//       logModeRead == 4 || logModeRead == 5 || logModeRead == 6 || 
//       logModeRead == 7 || logModeRead == 99 || logModeRead == 8) {
      
//       logMode = logModeRead;  // Set the logMode if it's valid
      
//   } else {
//       DEBUG_PRINTLN("Error: Invalid LOG_MODE value in CONFIG.TXT! Using default value.");
//       configError = true;
//   }

//   logStartHr      = settings[1]; //no check
//   logEndHr        = settings[2]; //no check
//   logStartDay     = settings[3]; //no check
//   epochSleep      = settings[4]; //no check

//   int ledBlinkRead        = settings[5];
//   int measureBatteryRead  = settings[6];
//   if (ledBlinkRead == 0 || ledBlinkRead == 1) {
//       ledBlink = (settings[5] != 0);
//   } else {
//       DEBUG_PRINTLN("Warning: Invalid value for LED_INDICATORS. Retaining default.");
//       configError = true;
//   }
  
//   if (measureBatteryRead == 0 || measureBatteryRead == 1) {
//       measureBattery = (settings[6] != 0);
//   } else {
//       DEBUG_PRINTLN("Warning: Invalid value for MEASURE_BATTERY. Retaining default.");
//       configError = true;
//   }
  
//   configError |= checkAndAssign(logGPS, settings[7], "ENABLE_GPS");
//   configError |= checkAndAssign(logGLO, settings[8], "ENABLE_GLO");
//   configError |= checkAndAssign(logGAL, settings[9], "ENABLE_GAL");
//   configError |= checkAndAssign(logBDS, settings[10], "ENABLE_BDS");
//   configError |= checkAndAssign(logQZSS, settings[11], "ENABLE_QZSS");
//   configError |= checkAndAssign(logNav, settings[12], "ENABLE_NAV_SFRBX");

//   int measurementRateRead = settings[14];
//   if (measurementRateRead >= 1 && measurementRateRead <=60) {
//     measurementRate = settings[14];
//   } else {
//     DEBUG_PRINTLN("Warning: Invalid value for MEASUREMENT_RATE. Retaining default.");
//     configError = true;
//   }
  
//   // Validate winterInterval
//   int winterIntervalRead = settings[16];
//   if (winterIntervalRead > 0) {
//       winterInterval = winterIntervalRead;
//   } else {
//       DEBUG_PRINTLN("Warning: Invalid WINTER_INTERVAL. Retaining default.");
//       configError = true;
//   }
  
//   // Validate startMonth
//   int startMonthRead = settings[17];
//   if (startMonthRead >= 1 && startMonthRead <= 12) {
//       startMonth = startMonthRead;
//   } else {
//       DEBUG_PRINTLN("Warning: Invalid START_MONTH. Retaining default.");
//       configError = true;
//   }
  
//   // Validate endMonth
//   int endMonthRead = settings[18];
//   if (endMonthRead >= 1 && endMonthRead <= 12) {
//       endMonth = endMonthRead;
//   } else {
//       DEBUG_PRINTLN("Warning: Invalid END_MONTH. Retaining default.");
//       configError = true;
//   }
  
//   // Validate startDay (cap at 28)
//   int startDayRead = settings[19];
//   if (startDayRead >= 1) {
//       startDay = (startDayRead > 28) ? 28 : startDayRead;
//   } else {
//       DEBUG_PRINTLN("Warning: Invalid START_DAY. Retaining default.");
//       configError = true;
//   }
  
//   // Validate endDay (cap at 28)
//   int endDayRead = settings[20];
//   if (endDayRead >= 1) {
//       endDay = (endDayRead > 28) ? 28 : endDayRead;
//   } else {
//       DEBUG_PRINTLN("Warning: Invalid END_DAY. Retaining default.");
//       configError = true;
//   }

//   // Validate TERMINAL_LOG
//   int terminalLogRead = settings[21];
//   if (terminalLogRead == 0 || terminalLogRead == 1) {
//       terminalLogging = (settings[21] != 0);
//   } else {
//       DEBUG_PRINTLN("Warning: Invalid value for TERMINAL_LOG. Retaining default.");
//       configError = true;
//   }
  
//   if (terminalLogging && measurementRate < 5) {
//     DEBUG_PRINTLN("Warning: Terminal logging disabled because measurementRate < 5s");
//     terminalLogging = false;
//   }

//   DEBUG_PRINT("Info: SD Settings from Device #: "); DEBUG_PRINTLN(stationName);
//   if (logMode == 1 ) {
//     DEBUG_PRINT("Log Mode 1 - Start/End Hours: "); DEBUG_PRINT(logStartHr); DEBUG_PRINT(", ");DEBUG_PRINTLN(logEndHr);
//   }
  
//   if (logMode == 3 ) {
//     DEBUG_PRINT("Log Mode 3 - Day of Month to Log: "); DEBUG_PRINTLN(logStartDay);
//   }

//   if (logMode == 4 ) {
//     DEBUG_PRINT("Log Mode 4 - Sleep Interval "); DEBUG_PRINTLN(epochSleep);
//   }

//   if (logMode == 6 ) {
//     DEBUG_PRINT("Log Mode 6 - Winter Interval "); DEBUG_PRINT(winterInterval); DEBUG_PRINT(" Summer months: "); DEBUG_PRINT(startMonth); DEBUG_PRINT(", "); DEBUG_PRINT(endMonth);
//   } else { 
//     DEBUG_PRINT(" - Log Mode: "); DEBUG_PRINTLN(logMode);
//   }
  
//   DEBUG_PRINT(" - Log Rate: "); DEBUG_PRINTLN(measurementRate);
//   DEBUG_PRINT(" - Log Battery?: "); DEBUG_PRINT(measureBattery); DEBUG_PRINT(" Shutdown V: "); DEBUG_PRINTLN(shutdownThreshold);
//   DEBUG_PRINT(" - Flash LED?: "); DEBUG_PRINTLN(ledBlink);
//   DEBUG_PRINT(" - Constellations: "); DEBUG_PRINT("GPS "); DEBUG_PRINT(logGPS); DEBUG_PRINT(" GLO "); DEBUG_PRINT(logGLO);
//   DEBUG_PRINT(" GAL "); DEBUG_PRINTLN(logGAL); DEBUG_PRINT("                   BDS "); DEBUG_PRINT(logBDS); DEBUG_PRINT(" QZSS "); 
//   DEBUG_PRINT(logQZSS); DEBUG_PRINT(" NAV "); DEBUG_PRINTLN(logNav);
  
//   if (!configFile.close()) {
//     DEBUG_PRINTLN("Warning: Failed to close config file.");
//     closeFailCounter++; // Count number of failed file closes
//     while(1) {
//     }
//   }

//   if (configError) {
//     DEBUG_PRINTLN("Warning: Invalid Config file value");
//     logDebug("CFG_FILE_ERROR");
//   }
  
// }

// bool checkAndAssign(int& variable, int readValue, const char* varName) {
//     if (readValue == 0 || readValue == 1) {
//         variable = readValue;
//         return false;
//     } else {
//         DEBUG_PRINT("Warning: Invalid value for ");
//         DEBUG_PRINT(varName);
//         DEBUG_PRINTLN(". Retaining default.");
//         return true;
//     }
// }

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
      blinkLed(5, 100, RED);
      delay(1000);
      return;
    }

    // Count the number of lines in the file
    int lineCount = 0;
    while ((n = dateFile.fgets(line, sizeof(line))) > 0) {
      lineCount++;
    }

    if (lineCount != EPOCH_FILE) {
      DEBUG_PRINT("Warning: detected an incompatable Date file. Your version # of dates: ");
      DEBUG_PRINT(lineCount); DEBUG_PRINT(" Should be: ");
      DEBUG_PRINTLN(EPOCH_FILE); DEBUG_PRINTLN("Warning: Not using dates.");
      blinkLed(5, 100, RED);
      delay(1000);

      if (!dateFile.close()) {
        DEBUG_PRINTLN("Warning: Failed to close config file.");
        closeFailCounter++; // Count number of failed file closes
        while(1) {
        }
      }
      
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
    configFile.print("LOG_START_HOUR_UTC(mode 1&8 only)=");
    configFile.println(logStartHr);
    configFile.print("LOG_END_HOUR_UTC(mode 1&8 only)=");
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
    configFile.print("OUTPUT_UART_RAWX=");
    configFile.println(terminalLogging ? "1" : "0");
    configFile.print("LOG_START_HOUR_TWO_UTC(mode 8 only)=");
    configFile.println(logStartHrTWO);
    configFile.print("LOG_END_HOUR_TWO_UTC(mode 8 only)=");
    configFile.println(logEndHrTWO);
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
