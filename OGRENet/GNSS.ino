void configureGNSS(){
  /////////GNSS SETTINGS
  myGNSS.disableUBX7Fcheck(); // RAWX data can legitimately contain 0x7F, so we need to disable the "7F" check in checkUbloxI2C
  myGNSS.setFileBufferSize(fileBufferSize); // setFileBufferSize must be called _before_ .begin
  
  if (myGNSS.begin() == false){
    DEBUG_PRINTLN(F("u-blox GNSS not detected at default I2C address. Please check wiring. Freezing..."));
    while (1);
  }
  
  myGNSS.newCfgValset8(UBLOX_CFG_SIGNAL_GPS_ENA, 0);   // Enable GPS
  myGNSS.addCfgValset8(UBLOX_CFG_SIGNAL_GLO_ENA, 1);   // Enable GLONASS
  myGNSS.addCfgValset8(UBLOX_CFG_SIGNAL_GAL_ENA, 1);   // Enable Galileo
  myGNSS.addCfgValset8(UBLOX_CFG_SIGNAL_BDS_ENA, 1);   // Enable BeiDou
  myGNSS.sendCfgValset8(UBLOX_CFG_SIGNAL_QZSS_ENA, 0); // Enable QZSS
  
  
  myGNSS.setI2COutput(COM_TYPE_UBX);                 // Set the I2C port to output UBX only (turn off NMEA noise)
  myGNSS.saveConfigSelective(VAL_CFG_SUBSEC_IOPORT); // Save (only) the communications port settings to flash and BBR
  myGNSS.setNavigationFrequency(1);                  // Produce one navigation solution per second (that's plenty for Precise Point Positioning)
  myGNSS.setAutoRXMSFRBX(true, false);               // Enable automatic RXM SFRBX messages 
  myGNSS.logRXMSFRBX();                              // Enable RXM SFRBX data logging
  myGNSS.setAutoRXMRAWX(true, false);                // Enable automatic RXM RAWX messages 
  myGNSS.logRXMRAWX();                               // Enable RXM RAWX data logging
}


void logGNSS(){
  ///////// DO GNSS STUFF
  myGNSS.checkUblox(); // Check for the arrival of new data and process it.
  while (myGNSS.fileBufferAvailable() >= sdWriteSize) { // Check to see if we have at least sdWriteSize waiting in the buffer
    petDog();
    uint8_t myBuffer[sdWriteSize]; // Create our own buffer to hold the data while we write it to SD card
    myGNSS.extractFileBufferData((uint8_t *)&myBuffer, sdWriteSize); // Extract exactly sdWriteSize bytes from the UBX file buffer and put them into myBuffer
    myFile.write(myBuffer, sdWriteSize); // Write exactly sdWriteSize bytes from myBuffer to the ubxDataFile on the SD card
    bytesWritten += sdWriteSize; // Update bytesWritten
    myGNSS.checkUblox(); // Check for the arrival of new data and process it if SD slow
  }

  ///////// PRINT BYTES WRITTEN (DEBUG MODE)
  #if DEBUG
    if (millis() > (lastPrint + 1000)) { // Print bytesWritten once per second
      Serial.print(F("The number of bytes written to SD card is ")); // Print how many bytes have been written to SD card
      Serial.println(bytesWritten);
      uint16_t maxBufferBytes = myGNSS.getMaxFileBufferAvail(); // Get how full the file buffer has been (not how full it is now)
      if (maxBufferBytes > ((fileBufferSize / 5) * 4)) { // Warn the user if fileBufferSize was more than 80% full
        Serial.println(F("Warning: the file buffer has been over 80% full. Some data may have been lost."));
      }
      lastPrint = millis(); // Update lastPrint
    }
  #endif
}

void closeGNSS(){
      uint16_t remainingBytes = myGNSS.fileBufferAvailable(); // Check if there are any bytes remaining in the file buffer
    while (remainingBytes > 0) { // While there is still data in the file buffer
      petDog();
      uint8_t myBuffer[sdWriteSize]; // Create our own buffer to hold the data while we write it to SD card
      uint16_t bytesToWrite = remainingBytes; // Write the remaining bytes to SD card sdWriteSize bytes at a time
      if (bytesToWrite > sdWriteSize) {
        bytesToWrite = sdWriteSize;
      }
      myGNSS.extractFileBufferData((uint8_t *)&myBuffer, bytesToWrite); // Extract bytesToWrite bytes from the UBX file buffer and put them into myBuffer
      myFile.write(myBuffer, bytesToWrite); // Write bytesToWrite bytes from myBuffer to the ubxDataFile on the SD card
      remainingBytes -= bytesToWrite; // Decrement remainingBytes
    }
}
