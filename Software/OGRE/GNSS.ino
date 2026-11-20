/*The functions on this page are adapted from DataLoggingExample3_RXM_SFRBX_and_RAWX  
 * Sparkfun OpenLog Artemis GNSS platform and GVMS.
 * see license for more details.
*/
void configureGNSS() {
#if DEBUG_GNSS
  gnss.enableDebugging(Serial);
#endif

// #if HARDWARE_VERSION == 1
//   gnss.disableUBX7Fcheck();
// #endif

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

  identifyGNSS(); // get module Type and version

  ///////// CONSTELLATION CONFIG (same code for F9P, F9T and X20P)
  if (initSetup) {
    // Each setting is sent on its own, so one rejected setting can't block the others.
    // Tries RAM + flash first, so settings survive power-off; falls back to RAM only.
    uint8_t failed = 0;
    auto trySetKey = [&](const char* name, uint32_t key, uint8_t value) {
      gnss.newCfgValset(VAL_LAYER_ALL);              // RAM + BBR + flash
      gnss.addCfgValset(key, value);
      bool ok = gnss.sendCfgValset();
      if (!ok) {                                     // flash rejected? apply to RAM only
        gnss.newCfgValset(VAL_LAYER_RAM);
        gnss.addCfgValset(key, value);
        ok = gnss.sendCfgValset();
      }
      if (!ok) {                                     // write rejected: is the setting in effect anyway?
        uint8_t cur;
        if (!gnss.getVal8(key, &cur, VAL_LAYER_RAM)) {
          DEBUG_PRINT(" (not supported)");           // firmware has no such setting: not a failure
        } else if (cur != value) {
          failed++;                                  // module is using a different value: real failure
        }
      }

      delay(500);                                    // let the receiver restart tracking
      petDog();
    };
    // One message for all signal settings. If any setting is rejected,
    // none is applied, so retry without GLONASS, then fall back to one at a time.
    auto sendBatch = [&](bool includeGlo) -> bool {
      gnss.newCfgValset(VAL_LAYER_ALL);              // RAM + BBR + flash
      gnss.addCfgValset(UBLOX_CFG_SIGNAL_GPS_ENA,  (uint8_t)logGPS);
      if (includeGlo) gnss.addCfgValset(UBLOX_CFG_SIGNAL_GLO_ENA, (uint8_t)logGLO);
      gnss.addCfgValset(UBLOX_CFG_SIGNAL_GAL_ENA,  (uint8_t)logGAL);
      gnss.addCfgValset(UBLOX_CFG_SIGNAL_BDS_ENA,  (uint8_t)logBDS);
      gnss.addCfgValset(UBLOX_CFG_SIGNAL_SBAS_ENA, (uint8_t)logSBAS);
      gnss.addCfgValset(UBLOX_CFG_SIGNAL_QZSS_ENA, (uint8_t)logQZSS);
      if (logL5 == 1) {
        gnss.addCfgValset(UBLOX_CFG_SIGNAL_GPS_L5_ENA,  (uint8_t)logL5);
        gnss.addCfgValset(UBLOX_CFG_SIGNAL_GAL_E5A_ENA, (uint8_t)logL5);
      }
      return gnss.sendCfgValset();
    };

    DEBUG_PRINTLN("Info: Configuring constellations.");
    if (sendBatch(true)) {
      DEBUG_PRINTLN(" - All settings OK");
    } else if (sendBatch(false)) {
      DEBUG_PRINTLN(" - All settings OK except GLO");
      uint8_t cur;                                   // GLO only counts as a failure if it isn't
      if (gnss.getVal8(UBLOX_CFG_SIGNAL_GLO_ENA, &cur, VAL_LAYER_RAM) && cur != (uint8_t)logGLO) {
        failed++;                                    // already set the way CONFIG asks
      }
    } else {
      DEBUG_PRINTLN(" - Batch rejected, setting one at a time");
      trySetKey("GPS",  UBLOX_CFG_SIGNAL_GPS_ENA,  (uint8_t)logGPS);
      trySetKey("GLO",  UBLOX_CFG_SIGNAL_GLO_ENA,  (uint8_t)logGLO);
      trySetKey("GAL",  UBLOX_CFG_SIGNAL_GAL_ENA,  (uint8_t)logGAL);
      trySetKey("BDS",  UBLOX_CFG_SIGNAL_BDS_ENA,  (uint8_t)logBDS);
      trySetKey("SBAS", UBLOX_CFG_SIGNAL_SBAS_ENA, (uint8_t)logSBAS);
      trySetKey("QZSS", UBLOX_CFG_SIGNAL_QZSS_ENA, (uint8_t)logQZSS);
      if (logL5 == 1) {
        trySetKey("GPS_L5",  UBLOX_CFG_SIGNAL_GPS_L5_ENA,  (uint8_t)logL5);
        trySetKey("GAL_E5A", UBLOX_CFG_SIGNAL_GAL_E5A_ENA, (uint8_t)logL5);
      }
    }
    delay(1000);                                     // one pause for the receiver to restart tracking
    petDog();

    if (failed) {                                    // record it, but keep logging
      char code[16];
      snprintf(code, sizeof(code), "CFG_FAIL_%u", failed);
      logDebug(code);
    }
  }
  initSetup = false;

  //////////////////////////////////////////
  gnss.setI2COutput(COM_TYPE_UBX);                 // Set the I2C port to output UBX only (no NMEA)
  gnss.saveConfigSelective(VAL_CFG_SUBSEC_IOPORT); // Save communications port settings to flash
  gnss.setNavigationFrequency(1);                  // Produce one navigation solution per second
  gnss.setAutoRXMRAWXrate(measurementRate, false); // Enable automatic RXM RAWX (RAW) messages 
  gnss.logRXMRAWX();                               // Enable RXM RAWX (RAW) data logging

  if (logNav == 1) {
    gnss.setAutoRXMSFRBXrate(measurementRate, false);  // Enable automatic RXM SFRBX (NAV) messages
    gnss.logRXMSFRBX();                                // Enable RXM SFRBX (NAV) data logging
  }

  gnss.setAutoPVTrate(true);
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
        digitalWrite(LED_LOG, HIGH);
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
        digitalWrite(LED_LOG, LOW);
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


// Runs once per boot, after gnss.begin(). Decides whether OGRE turns L5 on.
void identifyGNSS() {
  static bool done = false;
  if (done) return;
  done = true;

  logL5 = 0;

  const char* name = "UNKNOWN";
  bool haveInfo = gnss.getModuleInfo();
  if (haveInfo) name = gnss.getModuleName();

  // What the module is running right now (loaded from its flash/factory settings)
  uint8_t l5Now = 0;
  bool l5KeyExists = gnss.getVal8(UBLOX_CFG_SIGNAL_GPS_L5_ENA, &l5Now, VAL_LAYER_RAM);
  if (l5KeyExists && strstr(name, "X20")) logL5 = 1;

  // Record what was detected in the debug file (one row per boot)
  char info[48];
  if (haveInfo) {
    snprintf(info, sizeof(info), "GNSS module: %s %s %d.%02d", name,
             gnss.getFirmwareType(), gnss.getFirmwareVersionHigh(), gnss.getFirmwareVersionLow());
  } else {
    snprintf(info, sizeof(info), "GNSS module: UNKNOWN");
  }
  DEBUG_PRINT("Info: "); DEBUG_PRINTLN(info);
  // logDebugLine(info);   // once per boot, its own line, not in the errorCode column
}
