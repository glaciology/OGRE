void configureSD() {
  ///////// SD CARD INITIALIZATION
  if (!sd.begin(SD_CONFIG)) { // 
    Serial.println("Warning: Card failed, or not present. Trying again...");
    delay(2000);

    if (!sd.begin(SD_CONFIG)) { // 
      Serial.println("Warning: Card failed, or not present. Restarting...");
      while (1) {
        delay(2000);
      }
    }
    else {
      Serial.println("Info: microSD initialized.");
    }
  }
  else {
    Serial.println("Info: microSD initialized.");
  }
}


void getFileToCompress(){
  if (!myFile.open("566big.ubx", O_READ)){
    Serial.println("warning: failed to open _filename_.ubx");
  }
}

void getFiletoWrite(){
  if(!myWriteFile.open("compressed_new2.comp", O_CREAT | O_WRITE)){
    Serial.println("warning: failed to create/open compressed.comp");
  }
}
