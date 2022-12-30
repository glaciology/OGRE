// Derek Pickell 12/22
// Testing SWARM Modem with OGRE board
// WITH ANTENNA SPLITTER, CAN WE DISABLE UBLOX AND HAVE ANTENNA PASSIVE? 

///////// LIBRARIES & OBJECT INSTANTIATIONS //////////
#include <SparkFun_Swarm_Satellite_Arduino_Library.h> // NOTE: in .h file modification needed: 
                                                      // define SWARM_M138_SOFTWARE_SERIAL_ENABLED
#include <SoftwareSerial.h>
SWARM_M138 mySwarm;
////////////////////////////////

///////// HARDWARE-SPECIFIC PINOUTS & OBJECTS ////////
const byte SWARM_RX  = 9;
const byte SWARM_TX  = 10;
const byte SWARM_CNTRL  = 19;     // ONLY WORKS WITH HARDWARE_VERSION == 1 BOARDS
SoftwareSerial mySerial(SWARM_RX, SWARM_TX);

///////// GLOBAL VARIABLES ///////////////////////////
char          message[100]     = "";
//EXISITING VARIABLES
int  stationName               = 0000;  
unsigned long bytesWritten     = 0;
unsigned long writeFailCounter = 0;
unsigned long closeFailCounter = 0;
//PROGRAM (NEED TO FIX)
volatile bool satFlag          = false;
uint32_t hold                  = 1800;       // hold message for 30 minutes (CUSTOM!!!)
float         voltageFinal     = 0;
int32_t       latitude         = 0;
int32_t       longitude        = 0;
int32_t       alt              = 0;


// Callback: printMessageSent will be called when a new unsolicited $TD SENT message arrives.
void printMessageSent(const int16_t *rssi_sat, const int16_t *snr, const int16_t *fdev, const uint64_t *msg_id) {
  Serial.print(F("New $TD SENT message received:"));
  Serial.print(F("  RSSI = "));
  Serial.print(*rssi_sat);
  Serial.print(F("  SNR = "));
  Serial.print(*snr);
  Serial.print(F("  FDEV = "));
  Serial.print(*fdev);
  Serial.print(F("  Message ID: "));
  serialPrintUint64_t(*msg_id);
  Serial.println();
  satFlag = true;
}


void sendTelemetry(){
  ////////// TURN ON MODEM POWER /////////////
  pinMode(SWARM_CNTRL, OUTPUT); // Enable modem power 
  digitalWrite(SWARM_CNTRL, HIGH);

  delay(1000);

  int bootCount = 0;
  while (!mySwarm.begin(mySerial) && bootCount < 4) { // If .begin failed, keep trying
    //petDog(); UNCOMMENT
    Serial.println(F("Could not communicate with the modem. It may still be booting..."));
    delay(2000);
    bootCount++;
  }

  if (bootCount == 4){
    Serial.println("Could not communicate with modem. Shutting Down");
    digitalWrite(SWARM_CNTRL, LOW);
  }

  // Wait until the modem has valid Date/Time
  Swarm_M138_DateTimeData_t dateTime;
  Swarm_M138_Error_e err = mySwarm.getDateTime(&dateTime);

  while (err != SWARM_M138_SUCCESS) {
    Serial.print(F("Swarm communication error: "));
    Serial.print((int)err);
    Serial.print(F(" : "));
    Serial.println(mySwarm.modemErrorString(err)); // Convert the error into printable text
    Serial.println(F("The modem may not have acquired a valid GPS date/time reference..."));
    delay(2000);
    err = mySwarm.getDateTime(&dateTime);
  }

  // Set up the callback for the unsolicited $TD SENT messages. Call printMessageSent when a new message arrives
  mySwarm.setTransmitDataCallback(&printMessageSent);

  // SEND MESSAGE 
  uint64_t id;
  
//  sprintf(message, "%04d_%lu_%lu_%lu_%04f_%d_%d_%d", stationName, bytesWritten, writeFailCounter, 
//          closeFailCounter, voltageFinal, latitude, longitude, alt);
    
  sprintf(message, "%04d_%lu_%lu_%lu_%04f_%d_%d_%d", stationName, bytesWritten, writeFailCounter, 
      closeFailCounter, voltageFinal, 90, 74, 3222);
      
  Swarm_M138_Error_e err3 = mySwarm.transmitTextHold(message, &id, hold); // Include a hold duration

  // CHECK IF QUEUED PROPERLY: 
  if (err3 == SWARM_M138_SUCCESS) {
    Serial.print(F("The message has been added to the transmit queue. The message ID is "));
    serialPrintUint64_t(id);
    Serial.println();
  }
  else {
    Serial.print(F("Swarm communication error: "));
    Serial.print((int)err3);
    Serial.print(F(" : "));
    Serial.print(mySwarm.modemErrorString(err3)); // Convert the error into printable text
    if (err == SWARM_M138_ERROR_ERR) { // If we received a command error (ERR), print it
      Serial.print(F(" : "));
      Serial.print(mySwarm.commandError); 
      Serial.print(F(" : "));
      Serial.println(mySwarm.commandErrorString((const char *)mySwarm.commandError)); 
    }
    else
      Serial.println();
    // logDebug("SatQueueFail");
    digitalWrite(SWARM_CNTRL, LOW);
    satFlag = false;
    return;
  }

  unsigned long loopStartTime = millis();
  while (!satFlag && millis() - loopStartTime < 1000UL * hold) {
    // petDog();
    Serial.println("waiting to send messages");
    delay(5000); 
  }

  if (!satFlag){
    Serial.println("timeout: not able to send to satellite");
  }

  // SHUT DOWN COMS
  digitalWrite(SWARM_CNTRL, LOW);
  satFlag = false;

}

void setup() {
  Serial.begin(115200);
  sendTelemetry();

}

void loop() {
//  mySwarm.checkUnsolicitedMsg();
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
