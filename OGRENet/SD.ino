
void configureSD(){
  ///////// SD CARD INITIALIZATION
  DEBUG_PRINTLN("Initializing SD card...");
  if (!sd.begin(PIN_SD_CS, SD_SCK_MHZ(48))) { // https://github.com/sparkfun/Arduino_Apollo3/issues/370
    DEBUG_PRINTLN("Card failed, or not present. Freezing...");
    // don't do anything more:
    while (1);
  }
  DEBUG_PRINTLN("SD card initialized.");

  // Create or open a file called "RAWX.ubx" on the SD card.
  // If the file already exists, the new data is appended to the end of the file.
  // If using longer logging timescales, consider creating new file each time?
  myFile = sd.open("RAWX.UBX", FILE_WRITE);
  if (!myFile) {
    DEBUG_PRINTLN(F("Failed to create UBX data file! Freezing..."));
    while (1);
  }
}
