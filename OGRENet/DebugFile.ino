// Create debugging log file
void createDebugFile() {
  if (!debugFile.open("debug.csv", O_CREAT | O_APPEND | O_WRITE)) { // Create file if new, Append to end, Open to Write
    DEBUG_PRINTLN("Warning: Failed to create debug file.");
    return;
  }
  else {
    DEBUG_PRINTLN("Info: Created debug file"); 
  }

  // Write header to file
  debugFile.println("datetime,online_gnss,rtcDrift, maxBufferBytes, wdtCounter Max");

  // Sync the debug file
  if (!debugFile.sync()) {
    DEBUG_PRINTLN("Warning: Failed to sync debug file.");
  }

  // Update the file create timestamp
  //updateFileCreate(&debugFile);

  // Close log file
  if (!debugFile.close()) {
    DEBUG_PRINTLN("Warning: Failed to close debug file.");
  }
}

// Log debugging information
void logDebug() {
  // Increment debug counter
  debugCounter++;

  // Open debug file for writing
  if (!debugFile.open("debug.csv", O_APPEND | O_WRITE))
  {
    DEBUG_PRINTLN("Warning: Failed to open debug file.");
    return;
  }
  else {
    DEBUG_PRINTLN("Info: Opened "); DEBUG_PRINTLN("debug.csv");
  }

  // Create datetime string
  char dateTime[30];
  sprintf(dateTime, "20%02d-%02d-%02d %02d:%02d:%02d",
          rtc.year, rtc.month, rtc.dayOfMonth,
          rtc.hour, rtc.minute, rtc.seconds);

  // Log debugging information
  debugFile.print(dateTime);            debugFile.print(",");
  debugFile.print(online.gnss);         debugFile.print(",");
  debugFile.print(online.logGnss);      debugFile.print(",");
  debugFile.print(rtcSyncFlag);         debugFile.print(",");
  debugFile.print(rtcDrift);            debugFile.print(",");
  debugFile.print(bytesWritten);        debugFile.print(",");
  debugFile.print(maxBufferBytes);      debugFile.print(",");
  debugFile.print(wdtCounterMax);       debugFile.print(",");
  debugFile.print(writeFailCounter);    debugFile.print(",");
  debugFile.print(syncFailCounter);     debugFile.print(",");
  debugFile.print(closeFailCounter);    debugFile.print(",");
  debugFile.print(analogRead(ADC_INTERNAL_VSS));
  debugFile.println(debugCounter);

  // Sync the debug file
  if (!debugFile.sync())
  {
    DEBUG_PRINTLN("Warning: Failed to sync debug file.");
    syncFailCounter++; // Count number of failed file syncs
  }

  // Update file access timestamps
  //updateFileAccess(&debugFile);

  // Close the debug file
  if (!debugFile.close()) {
    DEBUG_PRINTLN("Warning: Failed to close debug file.");
    closeFailCounter++; // Count number of failed file closes
  }
}
