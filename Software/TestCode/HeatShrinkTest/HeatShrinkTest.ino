// Demo Code adapted from Heatshrink (Copyright (c) 2013-2015, Scott Vokes <vokes.s@gmail.com>)
// Derek Pickell
// NOTE: in heatshrink_config.h, must define the following: 
/*
 * HEATSHRINK_DYNAMIC_ALLOC 0 => ENSURES STATIC ALLOCATION
 * HEATSHRINK_STATIC_INPUT_BUFFER_SIZE => 512+ BYTES (larger buffer = more efficient)
 * HEATSHRINK_STATIC_WINDOW_BITS 10 (2^W =1024 bytes)
 * HEATSHRINK_STATIC_LOOKAHEAD_BITS 4 (2^L bits)
 * 
 * Total memory: 16 + 2 * 2^W bytes + 2*2^W
 * ARTEMIS/APOLLO3 has 384KB RAM
 */

#include <stdint.h>
#include <ctype.h>
#include <heatshrink_encoder.h>

// Arduino Libraries
#include <SPI.h>
#include <SdFat.h>

SdFs sd;
FsFile myFile;
FsFile myWriteFile;
SPIClass mySpi(3);  // SPI BUS ON OGRE

#define SD_CONFIG SdSpiConfig(41, DEDICATED_SPI, SD_SCK_MHZ(24), &mySpi)
#define arduinoLED            33   // OGRE

#if HEATSHRINK_DYNAMIC_ALLOC
#error HEATSHRINK_DYNAMIC_ALLOC must be false for static allocation test suite.
#endif

#define HEATSHRINK_DEBUG
static heatshrink_encoder hse;

void setup() {
  pinMode(arduinoLED, OUTPUT);       // Configure the onboard LED for output
  digitalWrite(arduinoLED, HIGH);    // default to LED ON
  pinMode(18, OUTPUT);              
  digitalWrite(18, HIGH);            // Turn on SD Power Bus
  delay(1);
  mySpi.begin();
  delay(1);
  Serial.begin(115200);
  delay(1000);
  Serial.println("Compression Testing");
  configureSD();
  getFileToCompress();
  getFiletoWrite();

  #define BUFSIZE 1024  
  uint32_t input_size  = BUFSIZE;
  uint32_t output_size = 2048;
  uint8_t readBuffer[BUFSIZE];
  uint8_t IBUFFER[BUFSIZE];
  uint8_t OBUFFER[2048];
  uint8_t WRITE_BUFFER[BUFSIZE];
  uint32_t counter = 0;                 // counts # times buffer is sunk
  uint32_t input_counter = 0;           // tracks total size in bytes of input
  uint32_t output_counter = 0;          // tracks total size in bytes of output
  
  heatshrink_encoder_reset(&hse);
  uint32_t polled = 0;                  // counter for bytes data OUTPUT
  size_t count = 0;                     // counter updated via reference for data processed

  uint32_t t1 = micros();
  
  while(myFile.available()) {
    myFile.read(&readBuffer,BUFSIZE);  // get BUFSIZE amt data, write to readBuffer

//    polled = 0;
//    count = 0;
    Serial.println("got next 1024 sized buffer");
    uint32_t sunk = 0;                 // counter for synced data 
    
    memcpy(IBUFFER, readBuffer, BUFSIZE);
    input_counter += BUFSIZE;
    
    while (sunk < input_size ) { 
      heatshrink_encoder_sink(&hse, &IBUFFER[sunk], input_size - sunk, &count);
      sunk+=count;
      Serial.print("Sunk: ");
      Serial.println(count);

      HSE_poll_res pres;
      do {
        pres = heatshrink_encoder_poll(&hse, &OBUFFER[polled], output_size - polled, &count);
        polled+=count;

        Serial.print("Polled: ");
        Serial.println(polled);

        if (polled >= BUFSIZE){
          //move BUFSIZE from OBUFFER TO SD CARD
          memcpy(WRITE_BUFFER, OBUFFER, polled);
          myWriteFile.write(WRITE_BUFFER, polled);
          myWriteFile.sync();
          Serial.print("copied x number to WRITE_BUFFER ");
          Serial.println(polled);
          output_counter += polled;
          polled = 0;
        }
//      output_counter += polled;
//      polled = 0;
      } while( pres == HSER_POLL_MORE);
    } 
    counter +=1;
//    Serial.print("Sunk buff count: ");
//    Serial.println(counter);

  } 
  heatshrink_encoder_finish(&hse);

//  polled = 0;                  // counter for bytes data OUTPUT
//  count = 0;                     // counter updated via reference for data processed
  
  HSE_poll_res pres;
  do {
    pres = heatshrink_encoder_poll(&hse, &OBUFFER[polled], output_size - polled, &count);
    polled+=count;
  } while (pres == HSER_POLL_MORE);
  memcpy(WRITE_BUFFER, OBUFFER, polled);
  myWriteFile.write(WRITE_BUFFER, polled);
  Serial.print("copied additional blocks to WRITE_BUFFER: ");
  Serial.println(polled);
  output_counter +=polled;

  uint32_t t2 = micros();
  
  Serial.println("finished");
  Serial.print("input bytes: ");
  Serial.println(input_counter);
  Serial.print("compressed: ");
  Serial.println(output_counter);
  Serial.print("Time to compress: ");Serial.println((t2-t1)/1e6,6);
  


  Serial.println("done");
  delay(2000);
  myWriteFile.close();
  delay(2000);
  mySpi.end();
  digitalWrite(18, LOW);
  digitalWrite(arduinoLED, LOW);
  
}

void loop() {
  delay(100);
}
