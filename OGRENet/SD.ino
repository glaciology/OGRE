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


void parseConfig() {
  
  int n;
  int i = 0;
  // open CONFIG file
  file.open("CONFIG.TXT", O_READ);
  
  // check for open error
  if (!file.isOpen()){
    DEBUG_PRINTLN("Warning: Could not open CONFIG.TXT");
  }

  // read lines from the file
  while ((n = file.fgets(line, sizeof(line))) > 0) {
    if (line[n - 1] == '\n') {
      // remove '\n'
      line[n-1] = 0;
      char* parse1 = strtok(line, "=");
      int hold = strtol(strtok(NULL,"="), NULL, 10);
      settings[i] = hold;
      i++;
      
    } else {
      // no '\n' - line too long or missing '\n' at EOF
      DEBUG_PRINTLN("Warning: Unable to Read CONFIG. Check formatting");
    }
  }
}

void getFileName(){
  for(uint8_t i=0; i<100; i++){
    logFileName[4] = i/10 + '0';
    logFileName[5] = i/10 + '0';
    if (!sd.exists(logFileName)){
      File = sd.open(logFileName, FILE_WRITE);
      break;
    }
  }
}
