
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
  // open test file
  file.open("CONFIG.TXT", O_READ);
  
  // check for open error
  if (!file.isOpen()){
    Serial.println("Warning: Could not open CONFIG.TXT");
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
      // handle error
    }
  }
}
