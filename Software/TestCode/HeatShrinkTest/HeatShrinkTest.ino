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
//const int sdWriteSize       = 512;
//const int fileBufferSize    = 16384;

#if HEATSHRINK_DYNAMIC_ALLOC
#error HEATSHRINK_DYNAMIC_ALLOC must be false for static allocation test suite.
#endif

#define HEATSHRINK_DEBUG
static heatshrink_encoder hse;

//#define BUFFER_SIZE 2048    // for compression
//uint8_t orig_buffer[BUFFER_SIZE];
//uint8_t comp_buffer[BUFFER_SIZE];
//uint8_t decomp_buffer[BUFFER_SIZE];
//  #define sdBUFSIZE 1024
//  uint8_t readBuffer[sdBUFSIZE];
//  uint32_t comp_size = BUFFER_SIZE;

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
  uint32_t counter = 0;
  uint32_t input_counter = 0;
  uint32_t output_counter = 0;

  heatshrink_encoder_reset(&hse);

  uint32_t polled = 0;                  // counter for bytes data OUTPUT
  size_t count = 0;                     // counter updated via reference for data processed
  
  while(myFile.available()) {
    myFile.read(&readBuffer,BUFSIZE);  // get BUFSIZE amt data, write to readBuffer

    uint32_t sunk = 0;                 // counter for synced data 
    
    memcpy(IBUFFER, readBuffer, BUFSIZE);
//    Serial.print("input: ");
//    Serial.println(BUFSIZE);
    input_counter += BUFSIZE;
    
    while (sunk < input_size ) { 
      heatshrink_encoder_sink(&hse, &IBUFFER[sunk], input_size - sunk, &count);
      sunk+=count;

      HSE_poll_res pres;
      do {
        pres = heatshrink_encoder_poll(&hse, &OBUFFER[polled], output_size - polled, &count);
        polled+=count;

        if (polled >= BUFSIZE){
          //move BUFSIZE from OBUFFER TO SD CARD
          memcpy(WRITE_BUFFER, OBUFFER, polled);
          myWriteFile.write(WRITE_BUFFER, polled);
          myWriteFile.sync();
//          Serial.print("copied x number to WRITE_BUFFER ");
//          Serial.println(polled);
          output_counter += polled;
          polled = 0;
        }
        
      } while( pres == HSER_POLL_MORE);
    } 
    counter +=1;
//    Serial.print("Sunk buff count: ");
//    Serial.println(counter);

  } 
  heatshrink_encoder_finish(&hse);

  HSE_poll_res pres;
  do {
    pres = heatshrink_encoder_poll(&hse, &OBUFFER[polled], output_size - polled, &count);
    polled+=count;
  } while (pres == HSER_POLL_MORE);
  memcpy(WRITE_BUFFER, OBUFFER, polled);
  myWriteFile.write(WRITE_BUFFER, polled);
  Serial.print("copied additional to WRITE_BUFFER: ");
  Serial.println(polled);
  output_counter +=polled;
  
  Serial.println("finished");
  Serial.print("input bytes: ");
  Serial.println(input_counter);
  Serial.print("compressed: ");
  Serial.println(output_counter);

//  while (myFile.available()) {
//    //petDog();
//    Serial.println("Begin");
//    uint32_t n = myFile.read(&readBuffer, sdBUFSIZE);
////    Serial.println((char *) readBuffer);
////    Serial.println(n);
////    count++;
////    memcpy(orig_buffer, &readBuffer, n);
////    memcpy(orig_buffer, test_data, 168);
//    if (n = sdBUFSIZE) { 
//      compress(readBuffer,n,comp_buffer,comp_size);
//      Serial.println(n);
//    } else { 
//      compress(readBuffer,n,comp_buffer,comp_size);
//    }
//    Serial.print("compressed: ");
//    Serial.println(comp_size);
// //   Serial.print((char *) comp_buffer);
//    if(!myWriteFile.write(comp_buffer, comp_size)){
//      Serial.println("error writing");
//    }
//    comp_size = 2048;
//    myWriteFile.sync();

//    blinkLed(1, 10);
//  }
//  myWriteFile.sync();

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