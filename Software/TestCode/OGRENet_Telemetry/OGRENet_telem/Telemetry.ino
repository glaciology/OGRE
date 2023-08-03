// Derek Pickell 12/22
// Testing SWARM Modem with OGRE board
// WITH ANTENNA SPLITTER, CAN WE DISABLE UBLOX AND HAVE ANTENNA PASSIVE?


///////// GLOBAL VARIABLES ///////////////////////////
char          message[100]     = "";
//PROGRAM (NEED TO FIX)
volatile bool satFlag          = false;

// Callback: printMessageSent will be called when a new unsolicited $TD SENT message arrives.
void printMessageSent(const int16_t *rssi_sat, const int16_t *snr, const int16_t *fdev, const uint64_t *msg_id) {
  DEBUG_PRINT(F("New $TD SENT message received:"));
  DEBUG_PRINT(F("  RSSI = "));
  DEBUG_PRINT(*rssi_sat);
  DEBUG_PRINT(F("  SNR = "));
  DEBUG_PRINT(*snr);
  DEBUG_PRINT(F("  FDEV = "));
  DEBUG_PRINT(*fdev);
  DEBUG_PRINT(F("  Message ID: "));
  serialPrintUint64_t(*msg_id);
  DEBUG_PRINTLN();
  messageSent = true;
}


void sendTelemetry() {
  ////////// TURN ON MODEM POWER /////////////
  pinMode(SWARM_CNTRL, OUTPUT); // Enable modem power
  digitalWrite(SWARM_CNTRL, HIGH);
  delay(10000);
  myUart.begin(115200);
  ///////////////////////////////////////////
  
  messageSent = false;

  int bootCount = 0;
  while (!mySwarm.begin(myUart)) { // If .begin failed, keep trying
    petDog();
    DEBUG_PRINTLN(F("Could not communicate with the modem. It may still be booting..."));
    delay(2000);
    bootCount++;

    if (bootCount > 15 ) { // timeout after 30 seconds
      DEBUG_PRINTLN("Could not communicate with modem. Shutting Down");
      digitalWrite(SWARM_CNTRL, LOW);
      return; // make sure this cancels entire function
    }
  }

  ////////// WAIT FOR VALID TIME /////////////
  Swarm_M138_DateTimeData_t dateTime;
  Swarm_M138_Error_e err = mySwarm.getDateTime(&dateTime);

  int errCount = 0;
  while (err != SWARM_M138_SUCCESS) {
    petDog();
    DEBUG_PRINTLN(F("Swarm communication error: "));
    DEBUG_PRINT((int)err);
    DEBUG_PRINT(F(" : "));
    DEBUG_PRINT(mySwarm.modemErrorString(err)); // Convert the error into printable text
    DEBUG_PRINTLN(F("The modem may not have acquired a valid GPS date/time reference..."));
    delay(2000);
    err = mySwarm.getDateTime(&dateTime);
    //logDebug("Modem couldn't acquire GPS");
    errCount++;

    if (errCount > 80 ) {  // timeout after 160 seconds
      DEBUG_PRINTLN("The modem couldn't fix a valid GPS reference. Shutting down.");
      digitalWrite(SWARM_CNTRL, LOW);
      return;
    }
  }

  ////////// SEND MESSAGE /////////////
  mySwarm.setTransmitDataCallback(&printMessageSent); // Callback for when sent
  uint64_t id;

  int battery2 = int(100*measBat());
  int temp2 = int(getInternalTemp()*100);
  DEBUG_PRINTLN(battery2);
  uint32_t end_time = rtc.getEpoch();
  
  sprintf(message, "%04d_%lu_%lu_%lu_%04d_%d_%d_%d_%d_%10u_%04d", stationName, bytesWritten, writeFailCounter,
          closeFailCounter, battery2, lat, lon, alt, siv, end_time, temp2);

  DEBUG_PRINTLN(message);
  Swarm_M138_Error_e err2 = mySwarm.transmitTextHold(message, &id, hold); // Include a hold duration
  delay(100);
  // CHECK IF QUEUED PROPERLY:
  if (err2 == SWARM_M138_SUCCESS) {
    Serial.print(F("The message has been added to the transmit queue. The message ID is "));
    serialPrintUint64_t(id);
    DEBUG_PRINTLN();
  }
  else {
    DEBUG_PRINTLN(F("Swarm communication error: "));
    DEBUG_PRINT((int)err2);
    DEBUG_PRINT(F(" : "));
    DEBUG_PRINTLN(mySwarm.modemErrorString(err2)); // Convert the error into printable text
    digitalWrite(SWARM_CNTRL, LOW);
    satFlag = false;
    return;
//    
//    if (err2 == SWARM_M138_ERROR_ERR) { // If we received a command error (ERR), print it
//      DEBUG_PRINT(F(" : "));
//      DEBUG_PRINT(mySwarm.commandError);
//      DEBUG_PRINT(F(" : "));
//      DEBUG_PRINTLN(mySwarm.commandErrorString((const char *)mySwarm.commandError));
//    }
//    else
//      DEBUG_PRINTLN();
//    // logDebug("SatQueueFail");
//    digitalWrite(SWARM_CNTRL, LOW);
//    satFlag = false;
//    return;
  }

  unsigned long loopStartTime = millis();
  uint16_t unsent;
  while (!satFlag && millis() - loopStartTime < 1000UL * hold) { // roll over!?, TEST THIS
    petDog();
    mySwarm.checkUnsolicitedMsg();
    DEBUG_PRINTLN("waiting to send messages");
    delay(5000);
    mySwarm.getUnsentMessageCount(&unsent);
    if (unsent == 0) {
      DEBUG_PRINTLN("message sent.");
      DEBUG_PRINTLN(unsent);
      satFlag == true;
      break;
    } else {
      DEBUG_PRINT(unsent);
      DEBUG_PRINTLN(" messages unsent");
    }
  }

  if (!satFlag) {
    DEBUG_PRINTLN("timeout: not able to send to satellite");
  }

  // SHUT DOWN COMS
  digitalWrite(SWARM_CNTRL, LOW);
  satFlag = false;

}

void serialPrintUint64_t(uint64_t theNum) {
  // Convert uint64_t to string
  // Based on printLLNumber by robtillaart
  // https://forum.arduino.cc/index.php?topic=143584.msg1519824#msg1519824

  char rev[21]; // Char array to hold to theNum (reversed order)
  char fwd[21]; // Char array to hold to theNum (correct order)
  unsigned int i = 0;
  if (theNum == 0ULL) // if theNum is zero, set fwd to "0"
  {
    fwd[0] = '0';
    fwd[1] = 0; // mark the end with a NULL
  }
  else
  {
    while (theNum > 0)
    {
      rev[i++] = (theNum % 10) + '0'; // divide by 10, convert the remainder to char
      theNum /= 10; // divide by 10
    }
    unsigned int j = 0;
    while (i > 0)
    {
      fwd[j++] = rev[--i]; // reverse the order
      fwd[j] = 0; // mark the end with a NULL
    }
  }

  Serial.print(fwd);
}
