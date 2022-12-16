// Demo Code for Heatshrink (Copyright (c) 2013-2015, Scott Vokes <vokes.s@gmail.com>)
// embedded compression library
// Craig Versek, Apr. 2016

#include <stdint.h>
#include <ctype.h>
#include <heatshrink_encoder.h>
#include <heatshrink_decoder.h>
// MY Libraries
#include <SPI.h>
#include <SdFat.h>

SdFs sd;
FsFile myFile;
SPIClass mySpi(3);

#define SD_CONFIG SdSpiConfig(41, DEDICATED_SPI, SD_SCK_MHZ(24), &mySpi)
#define arduinoLED            33   // OGRE
const int sdWriteSize       = 512;
const int fileBufferSize    = 16384;

#define BUFFER_SIZE 512    // for compression
uint8_t orig_buffer[BUFFER_SIZE];
uint8_t comp_buffer[BUFFER_SIZE];
uint8_t decomp_buffer[BUFFER_SIZE];

#if HEATSHRINK_DYNAMIC_ALLOC
#error HEATSHRINK_DYNAMIC_ALLOC must be false for static allocation test suite.
#endif

//#define HEATSHRINK_DEBUG
static heatshrink_encoder hse;
static heatshrink_decoder hsd;

//static void fill_with_pseudorandom_letters(uint8_t *buf, uint16_t size, uint32_t seed) {
//    uint64_t rn = 9223372036854775783; // prime under 2^64
//    for (int i=0; i<size; i++) {
//        rn = rn*seed + seed;
//        buf[i] = (rn % 26) + 'a';
//    }
//}


void setup() {
  pinMode(arduinoLED, OUTPUT);      // Configure the onboard LED for output
  digitalWrite(arduinoLED, LOW);    // default to LED off
  Serial.begin(115200);
  delay(1000);
  pinMode(18, OUTPUT);              // Turn on SD
  Serial.println("Compression Testing");
  configureSD();
  getFileToCompress();
  
  //write some data into the compression buffer
  #define sdBUFSIZE 40
  byte buffer[512];
  while (myFile.available()) {
    size_t n = myFile.readBytes(buffer, sdBUFSIZE);
    //sendBytes(buffer, n);
  }
  
  const char test_data[] = "\x01\x00\x00\x02\x00\x00\x03\x00\x00\x03\x00\x00\x04\x00\x00\x04\x00\x00\x03\x00\x00\x01\x00\x00\x04\x00\x00\x08\x00\x00\x01\x00\x00\x08\x00\x00\x07\x00\x00\x05\x00\x00";
  uint32_t orig_size = 42;//strlen(test_data);
  uint32_t comp_size   = BUFFER_SIZE; //this will get updated by reference
//  uint32_t decomp_size = BUFFER_SIZE; //this will get updated by reference
  memcpy(orig_buffer, test_data, orig_size);
  
  uint32_t t1 = micros();
  compress(orig_buffer,orig_size,comp_buffer,comp_size);
  uint32_t t2 = micros();
//  decompress(comp_buffer,comp_size,decomp_buffer,decomp_size);
//  uint32_t t3 = micros();
  
  Serial.print("Size of orginal data: ");Serial.println(orig_size);
  Serial.print("Size of compressed data: ");Serial.println(comp_size);
  float comp_ratio = ((float) orig_size / comp_size);
  Serial.print("Compression ratio: ");Serial.println(comp_ratio);
  Serial.print("Time to compress: ");Serial.println((t2-t1)/1e6,6);
//  Serial.print("Time to decompress: ");Serial.println((t3-t2)/1e6,6);
}

void loop() {
  delay(100);
}
