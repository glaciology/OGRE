
void configureSD(){
  ///////// SD CARD INITIALIZATION
  if (!sd.begin(SD_CONFIG)) { // 
    DEBUG_PRINTLN("Card failed, or not present. Trying again...");
    delay(2000);

    if (!sd.begin(SD_CONFIG)) { // 
      DEBUG_PRINTLN("Card failed, or not present...");
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
    DEBUG_PRINTLN("SD card initialized.");
    online.uSD = true;
  }
  
}
