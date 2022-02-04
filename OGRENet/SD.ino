void configureSD(){
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


void getConfig() {
  
  int n;
  int i = 0;
  // open CONFIG file
  configFile.open("CONFIG.TXT", O_READ);
  
  // check for open error
  if (!configFile.isOpen()){
    DEBUG_PRINTLN("Warning: Could not open CONFIG.TXT");
    DEBUG_PRINTLN("Warning: Using hard-coded settings");
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
  logMode = settings[0];
  logStartHr = settings[1];
  logEndHr = settings[2];
  logStartDay = settings[3];
  DEBUG_PRINTLN("Info: Settings read from SD:");
  DEBUG_PRINT("Info: Log Mode: "); DEBUG_PRINTLN(logMode);

  if (logMode == 1 ){
    DEBUG_PRINT("Info: Start/End Hours: "); DEBUG_PRINT(logStartHr); DEBUG_PRINT(", ");DEBUG_PRINTLN(logStartHr);
  }
  
  if (logMode == 4 ){
    DEBUG_PRINT("Day of Month to Log: "); DEBUG_PRINTLN(logStartDay);
  }
  
  if (!configFile.close()) {
    DEBUG_PRINTLN("Warning: Failed to close config file.");
    closeFailCounter++; // Count number of failed file closes
  }
}
