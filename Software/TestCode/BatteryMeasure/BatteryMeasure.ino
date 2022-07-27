/*
  Continously read and record battery voltage, measured across 68K/10K divider
*/
#include <SPI.h>
#include <SdFat.h>
#define LED 33
#define BAT 35       // ADC PIN
#define BAT_CNTRL 22 // Turns MOSFET ON (LOW)
SdFs sd;
FsFile myFile;
SPIClass mySpi(3);
const byte PIN_SD_CS              = 41;       //
#define SD_CONFIG SdSpiConfig(PIN_SD_CS, DEDICATED_SPI, SD_SCK_MHZ(24), &mySpi)
float gain                   = 17.2;          // Gain/offset for 68k/10k voltage divider battery voltage measure
float offset                 = 0.23;          // ADC range 0-2.0V
const byte PER_POWER          = 18; 

void setup()
{
  Serial.begin(115200);
  Serial.println("Analog Read");

  pinMode(BAT_CNTRL, OUTPUT);
  analogReadResolution(14); //Set resolution to 14 bit
  pinMode(LED, OUTPUT);

  configureSD();
}

void loop()
{
  digitalWrite(BAT_CNTRL, HIGH);
  digitalWrite(LED, HIGH);
  delay(1);
  int measure0 = analogRead(BAT);
  delay(10);
  int measure1 = analogRead(BAT);
  delay(1);
  digitalWrite(BAT_CNTRL, LOW);
  digitalWrite(LED, LOW);
  float vcc0 = (float)measure0 * gain / 16384.0 + offset; // convert to normal number
  float vcc1 = (float)measure1 * gain / 16384.0 + offset; // convert to normal number
  float voltageFinal = (vcc0 + vcc1) / 2;
  myFile.print(voltageFinal); myFile.print(",");
  Serial.println(voltageFinal);
  delay(1000);
  myFile.sync();
}

void configureSD() {
  ///////// SD CARD INITIALIZATION
  pinMode(PER_POWER, OUTPUT);
  digitalWrite(PER_POWER, HIGH);
  delay(500);
  mySpi.begin();
  delay(1);
  
  if (!sd.begin(SD_CONFIG)) { // 
    Serial.println("Warning: Card failed...");
    delay(2000);
  }
  else {
    Serial.println("Info: microSD initialized.");
  }

  if (!myFile.open("battery.csv", O_CREAT | O_APPEND | O_WRITE)) { // Create file if new, Append to end, Open to Write
    Serial.println("Warning: Failed to create battery file.");
    while(1);
  } else {
    Serial.println("created file");
  }
}
