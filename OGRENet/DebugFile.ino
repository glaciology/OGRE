void createDebugFile() {
  
  if (!debugFile.open("debug.csv", O_CREAT | O_APPEND | O_WRITE)) { // Create file if new, Append to end, Open to Write
    DEBUG_PRINTLN("Warning: Failed to create debug file.");
    return;
  }
  
  else {
    DEBUG_PRINTLN("Info: Created debug file"); 
  }

  // Write header to file
  debugFile.println("datetime, debugCounter, onlineGNSS, onlineLogGNSS, onlineSD, rtcSync, rtcDrift, bytesWritten, maxBufferBytes, wdtCounterMax, writeFailCounter, syncFailCounter, closeFailCounter, settings, Temperature, Battery");

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
  pinMode(BAT_CNTRL, OUTPUT);
  digitalWrite(BAT_CNTRL, HIGH);
  delay(1);
  int measure = analogRead(BAT);
  delay(1);
  digitalWrite(BAT_CNTRL, LOW);
  float vcc = (float)measure * converter / 16384.0; // convert to normal number
  
  return vcc;
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
  rtc.getTime();
  sprintf(dateTime, "20%02d-%02d-%02d %02d:%02d:%02d",
          rtc.year, rtc.month, rtc.dayOfMonth,
          rtc.hour, rtc.minute, rtc.seconds);

  // Log debugging information
  debugFile.print(dateTime);            debugFile.print(",");
  debugFile.print(debugCounter);        debugFile.print(",");
  debugFile.print(online.gnss);         debugFile.print(",");
  debugFile.print(online.logGnss);      debugFile.print(",");
  debugFile.print(online.uSD);          debugFile.print(",");
  debugFile.print(rtcSyncFlag);         debugFile.print(",");
  debugFile.print(rtcDrift);            debugFile.print(",");
  debugFile.print(bytesWritten);        debugFile.print(",");
  debugFile.print(maxBufferBytes);      debugFile.print(",");
  debugFile.print(wdtCounterMax);       debugFile.print(",");
  debugFile.print(writeFailCounter);    debugFile.print(",");
  debugFile.print(syncFailCounter);     debugFile.print(",");
  debugFile.print(closeFailCounter);    debugFile.print(",");
  debugFile.print(logMode);             debugFile.print(","); // refine this option
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

  // Close the debug file
  if (!debugFile.close()) {
    DEBUG_PRINTLN("Warning: Failed to close debug file.");
    closeFailCounter++; // Count number of failed file closes
  }
}
