/*The functions on this page are adapted from DataLoggingExample3_RXM_SFRBX_and_RAWX  
 * Sparkfun OpenLog Artemis GNSS platform and GVMS.
 * see license for more details.
*/
void configureGNSS() {
  ///////// SEND GNSS SETTING CONFIG TO UBLOX
  #if DEBUG_GNSS
  gnss.enableDebugging(Serial, true);     // Simpler DEBUG messages
  #endif
  
  gnss.disableUBX7Fcheck();               // RAWX data can legitimately contain 0x7F: So disable the "7F" check in checkUbloxI2C
  gnss.setFileBufferSize(fileBufferSize); // Allocate >2KB RAM for Buffer. CALL before .begin()

  ///////// ATTEMPT TO REACH UBLOX ON I2C
  if (gnss.begin(myWire) == false) {
    DEBUG_PRINTLN("Warning: UBLOX not detected at default I2C address. Reattempting...");
    delay(2000);

    if (gnss.begin(myWire) == false) {    // TRYING AGAIN
      DEBUG_PRINTLN("Warning: UBLOX not detected at default I2C address. Waiting for reset.");
      online.gnss = false;
      logDebug("I2C");
      zedPowerOff();
      while(1) {
        blinkLed(3, 250);
        delay(2000);
      }
    }
    else {
      DEBUG_PRINTLN("Info: UBLOX initialized.");
      online.gnss = true;
    }
  }
  else {
    DEBUG_PRINTLN("Info: UBLOX initialized.");
    online.gnss = true;
  }
  
  //////////////////////////////////////////
  if (initSetup) {                                                       // ONLY run this once, during initialization
    bool success = true;    
    success &= gnss.newCfgValset8(UBLOX_CFG_SIGNAL_GPS_ENA, logGPS);     // Enable GPS (define in USER SETTINGS)
    success &= gnss.addCfgValset8(UBLOX_CFG_SIGNAL_GLO_ENA, logGLO);     // Enable GLONASS
    success &= gnss.addCfgValset8(UBLOX_CFG_SIGNAL_GAL_ENA, logGAL);     // Enable Galileo
    success &= gnss.addCfgValset8(UBLOX_CFG_SIGNAL_BDS_ENA, logBDS);     // Enable BeiDou
    success &= gnss.addCfgValset8(UBLOX_CFG_SIGNAL_SBAS_ENA, logSBAS);   // Enable SBAS
    success &= gnss.sendCfgValset8(UBLOX_CFG_SIGNAL_QZSS_ENA, logQZSS);  // Enable QZSS

    if (logL5 == 1) { 
      success &= gnss.newCfgValset8(UBLOX_CFG_SIGNAL_GPS_L5_ENA, logL5);  // Enable GPS L5
      success &= gnss.sendCfgValset8(UBLOX_CFG_SIGNAL_GAL_E5A_ENA, logL5); // Enable Galileo L5
    }

    delay(2000);
    if (!success) {
      DEBUG_PRINTLN("Warning: GNSS CONSTELLATION CONFIG FAILED - ATTEMPTING AGAIN"); 
      delay(2000);
      bool success = true;
      success &= gnss.newCfgValset8(UBLOX_CFG_SIGNAL_GPS_ENA, logGPS);     // Enable GPS (define in USER SETTINGS)
      success &= gnss.addCfgValset8(UBLOX_CFG_SIGNAL_GLO_ENA, logGLO);     // Enable GLONASS
      success &= gnss.addCfgValset8(UBLOX_CFG_SIGNAL_GAL_ENA, logGAL);     // Enable Galileo
      success &= gnss.addCfgValset8(UBLOX_CFG_SIGNAL_BDS_ENA, logBDS);     // Enable BeiDou
      success &= gnss.addCfgValset8(UBLOX_CFG_SIGNAL_SBAS_ENA, logSBAS);   // Enable SBAS
      success &= gnss.sendCfgValset8(UBLOX_CFG_SIGNAL_QZSS_ENA, logQZSS);  // Enable QZSS
    
      if (logL5 == 1) { 
        success &= gnss.newCfgValset8(UBLOX_CFG_SIGNAL_GPS_L5_ENA, logL5);  // Enable GPS L5
        success &= gnss.sendCfgValset8(UBLOX_CFG_SIGNAL_GAL_E5A_ENA, logL5); // Enable Galileo L5
      }

      delay(2000);
      if (!success) {
        DEBUG_PRINTLN("Warning: GNSS CONSTELLATION CONFIG FAILED AGAIN. Waiting for system reset.");
        logDebug("CFG");
        while(1){
          blinkLed(3, 250);
          delay(2000);
        }
      }
    }
  }

  initSetup = false;
  //////////////////////////////////////////
  
  gnss.setI2COutput(COM_TYPE_UBX);                 // Set the I2C port to output UBX only (no NMEA)
  gnss.saveConfigSelective(VAL_CFG_SUBSEC_IOPORT); // Save communications port settings to flash and BBR
  gnss.setNavigationFrequency(1);                  // Produce one navigation solution per second
  gnss.setAutoRXMRAWXrate(measurementRate, false); // Enable automatic RXM RAWX (RAW) messages 
  gnss.logRXMRAWX();                               // Enable RXM RAWX (RAW) data logging
  
  if (logNav == 1) {
    gnss.setAutoRXMSFRBXrate(measurementRate, false);  // Enable automatic RXM SFRBX (NAV) messages 
    gnss.logRXMSFRBX();                                // Enable RXM SFRBX (NAV) data logging
  }
  
  gnss.setAutoPVTrate(1);                          // Enable PVT messages for syncing RTC
}


void logGNSS() {
  ///////// RECEIVE AND LOG GNSS MESSAGES

  getLogFileName();

  if (!myFile.open(logFileNameDate, O_CREAT | O_APPEND | O_WRITE)) {
    DEBUG_PRINTLN(F("Warning: Failed to create UBX data file! Freezing..."));
    logDebug("GNSS_FILE_CREATE");
    while (1) {
      blinkLed(3, 250);
      delay(2000); 
    }
  }
  
  DEBUG_PRINT("Info: Creating new file: "); DEBUG_PRINTLN(logFileNameDate);

  rtc.getTime();
  if (!myFile.timestamp(T_CREATE, (rtc.year + 2000), rtc.month, rtc.dayOfMonth, rtc.hour, rtc.minute, rtc.seconds))
    {
      DEBUG_PRINT("Warning: Failed to update timestamp.");
    }
  
  
  bytesWritten      = 0;          // Reset debug counters
  writeFailCounter  = 0;
  syncFailCounter   = 0;

  gnss.clearFileBuffer();         // Clear file buffer
  gnss.clearMaxFileBufferAvail(); // Reset max file buffer size
  
  while(!alarmFlag) { 
    
    petDog();
    gnss.checkUblox();                                               // Check for arrival of new data and process it.
    
    while (gnss.fileBufferAvailable() >= sdWriteSize) {              // Check to see if we have at least sdWriteSize waiting in the buffer
      petDog();
  
      if (ledBlink){
        digitalWrite(LED, HIGH);
      }
      
      uint8_t myBuffer[sdWriteSize];                                 // Create our own buffer to hold the data while we write it to SD card
      gnss.extractFileBufferData((uint8_t *)&myBuffer, sdWriteSize); // Extract exactly sdWriteSize bytes from the UBX file buffer and put them into myBuffer
      
      if (!myFile.write(myBuffer, sdWriteSize)) {                    // Write exactly sdWriteSize bytes from myBuffer to the ubxDataFile on the SD card
          DEBUG_PRINTLN("Warning: Failed to write to log file!");
          writeFailCounter++;
      }
      
      bytesWritten += sdWriteSize;                                   // Update bytesWritten
      gnss.checkUblox();                                             // Check for the arrival of new data and process it if SD slow
  
      if (ledBlink){
        digitalWrite(LED, LOW);
      }
    }
  
    if (millis() - prevMillis > 5000) {                             // Save data every 5 seconds
      if (!myFile.sync()) {
        DEBUG_PRINTLN("Warning: Failed to sync log file!");
        syncFailCounter++;                                           // Count number of failed file syncs
      }
  
      DEBUG_PRINT(F("Bytes written to SD card: "));                  // Print how many bytes have been written to SD card
      DEBUG_PRINT(bytesWritten); DEBUG_PRINT(" Buffer: ");
      
      maxBufferBytes = gnss.getMaxFileBufferAvail();                 // Get how full the file buffer has been (not how full it is now)
      DEBUG_PRINTLN(maxBufferBytes);
      if (maxBufferBytes > ((fileBufferSize / 5) * 4)) {             // Warn the user if fileBufferSize was more than 80% full
          DEBUG_PRINTLN("Warning: the file buffer > 80% full. Some data may have been lost.");
      }
      
      prevMillis = millis();
    }
  }
}

void closeGNSS() {
    ///////// CALL ONCE DONE LOGGING GNSS
    uint16_t remainingBytes = gnss.fileBufferAvailable();         // Check if there are any bytes remaining in the file buffer
    while (remainingBytes > 0) {                                  // While there is still data in the file buffer
      petDog();
      uint8_t myBuffer[sdWriteSize];                              // Create our own buffer to hold the data while we write it to SD card
      uint16_t bytesToWrite = remainingBytes;                     // Write the remaining bytes to SD card sdWriteSize bytes at a time
      if (bytesToWrite > sdWriteSize) {
        bytesToWrite = sdWriteSize;
      }
      gnss.extractFileBufferData((uint8_t *)&myBuffer, bytesToWrite); // Extract bytesToWrite bytes from the UBX file buffer and put them into myBuffer
      myFile.write(myBuffer, bytesToWrite);                       // Write bytesToWrite bytes from myBuffer to the ubxDataFile on the SD card
      remainingBytes -= bytesToWrite;                             // Decrement remainingBytes
    }

    if (!myFile.sync()) {
      DEBUG_PRINTLN("Warning: Failed to sync log file!");
      syncFailCounter++;                                          // Count number of failed file syncs
    }

    delay(200); // CRITICAL - GIVE SD TIME TO WRITE

    if (!myFile.close()) {
      DEBUG_PRINTLN("Warning: failed to close log file.");
      closeFailCounter++; 
    }
}
