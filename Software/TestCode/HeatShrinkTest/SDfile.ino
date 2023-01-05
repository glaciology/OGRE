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
  if (!myFile.open("compressme.ubx", O_READ)){
    Serial.print("warning: failed to open compressme.ubx");
  }
}

void getFiletoWrite(){
  if(!myWriteFile.open("compressed.comp", O_CREAT | O_APPEND | O_WRITE))
}
