void createDebugFile() {
  
  if (!debugFile.open("debug.csv", O_CREAT | O_APPEND | O_WRITE)) { // Create file if new, Append to end, Open to Write
    DEBUG_PRINTLN("Warning: Failed to create debug file.");
    return;
  }
  
  else {
    DEBUG_PRINTLN("Info: Created debug file"); 
  }

  // Write header to file
  debugFile.println("datetime,onlineGNSS,onlineGNSSLog, rtcSync, rtcDrift, bytesWritten, maxBufferBytes, wdtCounterMax, writeFailCounter, syncFailCounter, closeFailCounter, Temperature, debugCounter, Battery");

  // Sync the debug file
  if (!debugFile.sync()) {
    DEBUG_PRINTLN("Warning: Failed to sync debug file.");
  }

  // Close log file
  if (!debugFile.close()) {
    DEBUG_PRINTLN("Warning: Failed to close debug file.");
  }
}


float measBat() {
  float converter = 17.5;    // THIS MUST BE TUNED DEPENDING ON WHAT RESISTORS USED IN VOLTAGE DIVIDER
  analogReadResolution(14); //Set resolution to 14 bit
  
  pinMode(BAT_CNTRL, HIGH);
  int measure = analogRead(BAT);
  delay(100);
  int measure2 = analogRead(BAT);
  delay(100);
  int measure3 = analogRead(BAT);
  pinMode(BAT_CNTRL, LOW);

  float avgMeas = ((float)measure + (float)measure2 + (float)measure3)/3.0;
  
  return avgMeas * converter / 16384.0;
}


void logDebug() {
  // Increment debug counter
  debugCounter++;

  analogReadResolution(14); //Set resolution to 14 bit

  if (!debugFile.open("debug.csv", O_APPEND | O_WRITE)) {
    DEBUG_PRINTLN("Warning: Failed to open debug file.");
    return;
  }
  
  else {
    DEBUG_PRINT("Info: Debug Dumped to: "); DEBUG_PRINTLN("debug.csv");
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
  debugFile.print(getInternalTemp(), 2); debugFile.print(",");
  debugFile.print(debugCounter); debugFile.print(",");
  if (measureBattery == true){
    debugFile.print(measBat());
  }
  debugFile.println();

  // Sync the debug file
  if (!debugFile.sync()) {
    DEBUG_PRINTLN("Warning: Failed to sync debug file.");
    syncFailCounter++; // Count number of failed file syncs
  }

  // Close the debug file
  if (!debugFile.close()) {
    DEBUG_PRINTLN("Warning: Failed to close debug file.");
    closeFailCounter++; // Count number of failed file closes
  }
}
