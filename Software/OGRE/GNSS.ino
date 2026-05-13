/*The functions on this page are adapted from DataLoggingExample3_RXM_SFRBX_and_RAWX  
 * Sparkfun OpenLog Artemis GNSS platform and GVMS.
 * see license for more details.
*/
void configureGNSS() {
#if DEBUG_GNSS
  gnss.enableDebugging(Serial, true);
#endif

#if HARDWARE_VERSION == 1
  gnss.disableUBX7Fcheck();
#endif

  gnss.setFileBufferSize(fileBufferSize);

  ///////// ATTEMPT TO REACH UBLOX ON I2C
  if (gnss.begin(myWire) == false) {
    DEBUG_PRINTLN("Warning: UBLOX not detected at default I2C address. Reattempting...");
    delay(2000);
    petDog();
    if (gnss.begin(myWire) == false) {
      DEBUG_PRINTLN("Warning: UBLOX not detected at default I2C address. Waiting for reset.");
      online.gnss = false;
      logDebug("I2C");
      zedPowerOff();
      while (1) { blinkLed(3, 250, RED); delay(2000); }
    } else {
      DEBUG_PRINTLN("Info: UBLOX initialized.");
      online.gnss = true;
    }
  } else {
    DEBUG_PRINTLN("Info: UBLOX initialized.");
    online.gnss = true;
  }

  ///////// CONSTELLATION CONFIG
  if (initSetup) {

#if HARDWARE_VERSION == 1
    // ---- v2 API: batch VALSET; retry once on failure ----
    auto sendV1Config = [&]() -> bool {
      bool ok = true;
      ok &= gnss.newCfgValset8(UBLOX_CFG_SIGNAL_GPS_ENA, logGPS);
      ok &= gnss.addCfgValset8(UBLOX_CFG_SIGNAL_GLO_ENA, logGLO);
      ok &= gnss.addCfgValset8(UBLOX_CFG_SIGNAL_GAL_ENA, logGAL);
      ok &= gnss.addCfgValset8(UBLOX_CFG_SIGNAL_BDS_ENA, logBDS);
      ok &= gnss.addCfgValset8(UBLOX_CFG_SIGNAL_SBAS_ENA, logSBAS);
      ok &= gnss.sendCfgValset8(UBLOX_CFG_SIGNAL_QZSS_ENA, logQZSS);

      if (logL5 == 1) {
        ok &= gnss.newCfgValset8(UBLOX_CFG_SIGNAL_GPS_L5_ENA, logL5);
        ok &= gnss.sendCfgValset8(UBLOX_CFG_SIGNAL_GAL_E5A_ENA, logL5);
      }
      return ok;
    };

    bool success = sendV1Config();
    delay(2000);
    petDog();

    if (!success) {
      DEBUG_PRINTLN("Warning: GNSS CONSTELLATION CONFIG FAILED - ATTEMPTING AGAIN");
      delay(2000);
      petDog();
      success = sendV1Config();
      delay(2000);
      petDog();

      if (!success) {
        DEBUG_PRINTLN("Warning: GNSS CONSTELLATION CONFIG FAILED AGAIN. Waiting for system reset.");
        logDebug("CFG");
        while (1) { blinkLed(3, 250, RED); delay(2000); }
      }
    }

#elif HARDWARE_VERSION == 2
    // ---- v3 API: X20P needs keys set individually; GLO can't be reconfigured ----
    auto trySetKey = [&](const char* name, uint32_t key, uint8_t value) -> bool {
      gnss.newCfgValset(VAL_LAYER_RAM);
      gnss.addCfgValset(key, value);
      bool ok = gnss.sendCfgValset();
      DEBUG_PRINT("  "); DEBUG_PRINT(name);
      DEBUG_PRINTLN(ok ? ": OK" : ": FAILED");
      petDog();
      return ok;
    };

    DEBUG_PRINTLN("Info: Configuring constellations:");
    trySetKey("GPS",  UBLOX_CFG_SIGNAL_GPS_ENA,  (uint8_t)logGPS);
    // GLO skipped: X20P rejects explicit set; factory default is enabled
    trySetKey("GAL",  UBLOX_CFG_SIGNAL_GAL_ENA,  (uint8_t)logGAL);
    trySetKey("BDS",  UBLOX_CFG_SIGNAL_BDS_ENA,  (uint8_t)logBDS);
    trySetKey("SBAS", UBLOX_CFG_SIGNAL_SBAS_ENA, (uint8_t)logSBAS);
    trySetKey("QZSS", UBLOX_CFG_SIGNAL_QZSS_ENA, (uint8_t)logQZSS);

    if (logL5 == 1) {
      trySetKey("GPS_L5",  UBLOX_CFG_SIGNAL_GPS_L5_ENA,  (uint8_t)logL5);
      trySetKey("GAL_E5A", UBLOX_CFG_SIGNAL_GAL_E5A_ENA, (uint8_t)logL5);
    }
    // No retry/halt for X20P — individual key failures are expected and non-fatal
#endif
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

#if HARDWARE_VERSION == 1
  gnss.setAutoPVTrate(1);
#elif HARDWARE_VERSION == 2
  gnss.setAutoPVTrate(true);
#endif

  DEBUG_PRINTLN("Info: GNSS configured.");
}

void logGNSS() {
  ///////// RECEIVE AND LOG GNSS MESSAGES

  getLogFileName();

  if (!myFile.open(logFileNameDate, O_CREAT | O_APPEND | O_WRITE)) {
    DEBUG_PRINTLN(F("Warning: Failed to create UBX data file! Freezing..."));
    logDebug("GNSS_FILE_CREATE");
    while (1) {
      blinkLed(3, 250, RED);
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

      if (terminalLogging) {
        for (int j = 0; j < sdWriteSize; j++) {
            Serial.write(myBuffer[j]);  // Print raw UBX bytes to terminal
        }
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

      if (terminalLogging) {
        for (int j = 0; j < bytesToWrite; j++) {
            Serial.write(myBuffer[j]);  // Print raw UBX bytes to terminal
//            Serial.write(myBuffer, bytesToWrite);  // Writes the whole chunk at once

        }
      }
      
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
