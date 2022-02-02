
void configureSD(){
  ///////// SD CARD INITIALIZATION
  if (!sd.begin(SD_CONFIG)) { // https://github.com/sparkfun/Arduino_Apollo3/issues/370
    DEBUG_PRINTLN("Card failed, or not present. Freezing...");
    while (1); 
  }
  DEBUG_PRINTLN("SD card initialized.");

  // Create a new log file and open for writing
  // O_CREAT  - Create the file if it does not exist
  // O_APPEND - Seek to the end of the file prior to each write
  // O_WRITE  - Open the file for writing
  myFile.open("RAWX.UBX", O_CREAT | O_APPEND | O_WRITE);
  if (!myFile) {
    DEBUG_PRINTLN(F("Failed to create UBX data file! Freezing..."));
    while (1); 
  }
}
