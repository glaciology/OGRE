/*
  Analog to digital conversion
  Hardware Connections:
  Upload code
  Use a trimpot or other device to send a 0 to 3.3V (no 5V!) signal to pin 'A16'
  ADC can resolve up to 2V but is 3.3V safe
*/

#define LED 33
#define BAT 32       // ADC PIN
#define BAT_CNTRL 22 // Turns MOSFET ON (LOW)

void setup()
{
  Serial.begin(115200);
  Serial.println("Analog Read");

  pinMode(BAT_CNTRL, OUTPUT);

  analogReadResolution(14); //Set resolution to 14 bit
}

void loop()
{
  digitalWrite(BAT_CNTRL, HIGH);

  int myValue1 = analogRead(BAT); //Automatically sets pin to analog input
  Serial.print("A32: ");
  Serial.print(myValue1);

  float vcc = (float)myValue1 * 17.5 / 16384.0; //Convert 1/3 VCC to VCC
  Serial.print(" VCC: ");
  Serial.print(vcc, 2);
  Serial.print("V");

  int vss = analogRead(ADC_INTERNAL_VSS); //Read internal VSS (should be 0)
  Serial.print("\tvss: ");
  Serial.print(vss);

  Serial.println();
  digitalWrite(BAT_CNTRL, LOW);
  delay(1000);
//
//  int myValue12 = analogRead(BAT); //Automatically sets pin to analog input
//  Serial.print("A32: ");
//  Serial.print(myValue12);
//
//  float vcc1 = (float)myValue12 * 17.5 / 16384.0; //Convert 1/3 VCC to VCC
//  Serial.print(" VCC: ");
//  Serial.print(vcc1, 2);
//  Serial.print("V");
//
//  int vss2 = analogRead(ADC_INTERNAL_VSS); //Read internal VSS (should be 0)
//  Serial.print("\tvss: ");
//  Serial.print(vss2);
//
//  Serial.println();
//  delay(1000);
}
