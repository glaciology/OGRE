/* Code adapted from GVMS.
 *  See license for more details. 
 */

void createDebugFile() {
  sprintf(debugFileName, "debug_%s.csv", stationName);  // Generate "debug_1234.csv", e.g.

  if (!debugFile.open(debugFileName, O_CREAT | O_APPEND | O_WRITE)) { // Create file if new, Append to end, Open to Write
    DEBUG_PRINTLN("Warning: Failed to create debug file.");
    return;
  }
  
  else {
    DEBUG_PRINTLN("Info: Created debug file"); 
  }

  volatile uint32_t *stat_register = (uint32_t *)STAT_REGISTER_ADDRESS;
  uint32_t stat_value = *stat_register;
  String STAT_CODES = getStatusCodes(stat_value);

  // Write header to file
  debugFile.println(SOFTWARE_VERSION"_datetime, debugCounter, onlineGNSS, onlineSD, rtcSync, rtcDrift, bytesWritten,"
  "maxBufferBytes, wdtCounterMax, writeFailCounter, syncFailCounter, closeFailCounter, LogMode, errorCode," 
  "lowBattery, Temperature, Battery, " + STAT_CODES);

  // Sync the debug file
  if (!debugFile.sync()) {
    DEBUG_PRINTLN("Warning: Failed to sync debug file.");
  }

  // Close log file
  if (!debugFile.close()) {
    DEBUG_PRINTLN("Warning: Failed to close debug file.");
  }
}


void logDebug(const char* errorCode) {
  
  debugCounter++; // Increment debug counter

  if (!debugFile.open(debugFileName, O_APPEND | O_WRITE)) {
    DEBUG_PRINTLN("Warning: Failed to open debug file.");
    return;
  }
  
  else {
    DEBUG_PRINT("Info: Debug dumped to: ");
    DEBUG_PRINTLN(debugFileName);
  }

  // Create datetime string
  char dateTime[30];
  rtc.getTime();
  sprintf(dateTime, "20%02d-%02d-%02d %02d:%02d:%02d",
          rtc.year, rtc.month, rtc.dayOfMonth,
          rtc.hour, rtc.minute, rtc.seconds);

  // Log debugging information
  debugFile.print(dateTime);             debugFile.print(",");
  debugFile.print(debugCounter);         debugFile.print(",");
  debugFile.print(online.gnss);          debugFile.print(",");
  debugFile.print(online.uSD);           debugFile.print(",");
  debugFile.print(online.rtcSync);      debugFile.print(",");
  debugFile.print(rtcDrift);             debugFile.print(",");
  debugFile.print(bytesWritten);         debugFile.print(",");
  debugFile.print(maxBufferBytes);       debugFile.print(",");
  debugFile.print(wdtCounterMax);        debugFile.print(",");
  debugFile.print(writeFailCounter);     debugFile.print(",");
  debugFile.print(syncFailCounter);      debugFile.print(",");
  debugFile.print(closeFailCounter);     debugFile.print(",");
  debugFile.print(logMode);              debugFile.print(","); 
  debugFile.print(errorCode);            debugFile.print(",");
  debugFile.print(lowBatteryCounter);    debugFile.print(",");
  debugFile.print(getInternalTemp(), 2); debugFile.print(",");
  if (measureBattery == true){
    debugFile.print(measBat(), 2);
  }
  debugFile.println();

  // Sync the debug file
  if (!debugFile.sync()) {
    DEBUG_PRINTLN("Warning: Failed to sync debug file.");
    syncFailCounter++; // Count number of failed file syncs
  }

  // Update filestamp
  if (!debugFile.timestamp(T_ACCESS, (rtc.year + 2000), rtc.month, rtc.dayOfMonth, rtc.hour, rtc.minute, rtc.seconds))
  {
    DEBUG_PRINT("Warning: File update timestamp issue.");
  }

  // Close the debug file
  if (!debugFile.close()) {
    DEBUG_PRINTLN("Warning: Failed to close debug file.");
    closeFailCounter++; // Count number of failed file closes
  }
}
