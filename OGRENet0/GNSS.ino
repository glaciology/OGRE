void configureGNSS(){
  ///////// SEND GNSS SETTING CONFIG TO UBLOX
  //gnss.enableDebugging();               // Sparkfun_Gnss library debugging messages on Serial
  //gnss.enableDebugging(Serial, true); // Or, uncomment this line to enable only the important GNSS debug messages on Serial
  gnss.disableUBX7Fcheck();               // RAWX data can legitimately contain 0x7F: So disable the "7F" check in checkUbloxI2C
  gnss.setFileBufferSize(fileBufferSize); // Allocate >2KB RAM for Buffer. CALL before .begin()
  
  if (gnss.begin() == false){
    DEBUG_PRINTLN(F("UBLOX not detected at default I2C address. Freezing..."));
    while (1); // WDT will reset sys
  }

  if (firstConfig) {
    bool success = true;
    success &= gnss.newCfgValset8(UBLOX_CFG_SIGNAL_GPS_ENA, logGPS);    // Enable GPS (define in USER SETTINGS)
    success &= gnss.addCfgValset8(UBLOX_CFG_SIGNAL_GLO_ENA, logGLO);    // Enable GLONASS
    success &= gnss.addCfgValset8(UBLOX_CFG_SIGNAL_GAL_ENA, logGAL);    // Enable Galileo
    success &= gnss.addCfgValset8(UBLOX_CFG_SIGNAL_BDS_ENA, logBDS);    // Enable BeiDou
    success &= gnss.sendCfgValset8(UBLOX_CFG_SIGNAL_QZSS_ENA, logQZSS); // Enable QZSS
    if (!success){
      DEBUG_PRINTLN("GNSS CONSTELLATIONS FAILED TO CONFIGURE");
    }
  }
  
  gnss.setI2COutput(COM_TYPE_UBX);                 // Set the I2C port to output UBX only (no NMEA)
  gnss.saveConfigSelective(VAL_CFG_SUBSEC_IOPORT); // Save communications port settings to flash and BBR
  gnss.setNavigationFrequency(1);                  // Produce one navigation solution per second
  gnss.setAutoRXMRAWX(true, false);                // Enable automatic RXM RAWX (RAW) messages 
  gnss.logRXMRAWX();                               // Enable RXM RAWX (RAW) data logging
  if (logNav){
    gnss.setAutoRXMSFRBX(true, false);            // Enable automatic RXM SFRBX (NAV) messages 
    gnss.logRXMSFRBX();                           // Enable RXM SFRBX (NAV) data logging
  }
}

void logGNSS(){
  ///////// RECEIVE AND LOG GNSS MESSAGES
  gnss.checkUblox(); // Check for arrival of new data and process it.
  while (gnss.fileBufferAvailable() >= sdWriteSize) { // Check to see if we have at least sdWriteSize waiting in the buffer
    petDog();
    uint8_t myBuffer[sdWriteSize]; // Create our own buffer to hold the data while we write it to SD card
    gnss.extractFileBufferData((uint8_t *)&myBuffer, sdWriteSize); // Extract exactly sdWriteSize bytes from the UBX file buffer and put them into myBuffer
    
    if (!myFile.write(myBuffer, sdWriteSize)){
        DEBUG_PRINTLN("Warning: Failed to write to log file!");
    }
    
    //myFile.write(myBuffer, sdWriteSize); // Write exactly sdWriteSize bytes from myBuffer to the ubxDataFile on the SD card
    bytesWritten += sdWriteSize; // Update bytesWritten
    gnss.checkUblox(); // Check for the arrival of new data and process it if SD slow
  }

  if (millis() > (prevMillis + 10000)){ // Periodically save data
    myFile.sync();
    prevMillis = millis();
  }

  ///////// PRINT BYTES WRITTEN (DEBUG MODE)
  #if DEBUG
    if (millis() > (lastPrint + 1000)) { // Print bytesWritten once per second
      petDog();
      Serial.print(F("The number of bytes written to SD card is ")); // Print how many bytes have been written to SD card
      Serial.println(bytesWritten);
      uint16_t maxBufferBytes = gnss.getMaxFileBufferAvail(); // Get how full the file buffer has been (not how full it is now)
      if (maxBufferBytes > ((fileBufferSize / 5) * 4)) { // Warn the user if fileBufferSize was more than 80% full
        Serial.println(F("Warning: the file buffer has been over 80% full. Some data may have been lost."));
      }
      lastPrint = millis(); // Update lastPrint
    }
  #endif
}

void closeGNSS(){
    ///////// CALL ONCE DONE LOGGING GNSS
    uint16_t remainingBytes = gnss.fileBufferAvailable(); // Check if there are any bytes remaining in the file buffer
    while (remainingBytes > 0) { // While there is still data in the file buffer
      petDog();
      uint8_t myBuffer[sdWriteSize]; // Create our own buffer to hold the data while we write it to SD card
      uint16_t bytesToWrite = remainingBytes; // Write the remaining bytes to SD card sdWriteSize bytes at a time
      if (bytesToWrite > sdWriteSize) {
        bytesToWrite = sdWriteSize;
      }
      gnss.extractFileBufferData((uint8_t *)&myBuffer, bytesToWrite); // Extract bytesToWrite bytes from the UBX file buffer and put them into myBuffer
      myFile.write(myBuffer, bytesToWrite); // Write bytesToWrite bytes from myBuffer to the ubxDataFile on the SD card
      remainingBytes -= bytesToWrite; // Decrement remainingBytes
    }

    delay(100); // CRITICAL - GIVE SD TIME TO WRITE

    if (!myFile.close()){
      DEBUG_PRINTLN("Warning: failed to close log file.");
    }
    bytesWritten = 0;
    lastPrint = 0;
}
