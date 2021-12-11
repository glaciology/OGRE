void configureGNSS(){
  /////////GNSS SETTINGS
  myGNSS.disableUBX7Fcheck(); // RAWX data can legitimately contain 0x7F, so we need to disable the "7F" check in checkUbloxI2C
  myGNSS.setFileBufferSize(fileBufferSize); // setFileBufferSize must be called _before_ .begin
  
  if (myGNSS.begin() == false){
    DEBUG_PRINTLN(F("u-blox GNSS not detected at default I2C address. Please check wiring. Freezing..."));
    while (1);
  }
  
  myGNSS.newCfgValset8(UBLOX_CFG_SIGNAL_GPS_ENA, 0);   // Enable GPS
  myGNSS.addCfgValset8(UBLOX_CFG_SIGNAL_GLO_ENA, 1);   // Enable GLONASS
  myGNSS.addCfgValset8(UBLOX_CFG_SIGNAL_GAL_ENA, 1);   // Enable Galileo
  myGNSS.addCfgValset8(UBLOX_CFG_SIGNAL_BDS_ENA, 1);   // Enable BeiDou
  myGNSS.sendCfgValset8(UBLOX_CFG_SIGNAL_QZSS_ENA, 0); // Enable QZSS
  
  
  myGNSS.setI2COutput(COM_TYPE_UBX);                 // Set the I2C port to output UBX only (turn off NMEA noise)
  myGNSS.saveConfigSelective(VAL_CFG_SUBSEC_IOPORT); // Save (only) the communications port settings to flash and BBR
  myGNSS.setNavigationFrequency(1);                  // Produce one navigation solution per second (that's plenty for Precise Point Positioning)
  myGNSS.setAutoRXMSFRBX(true, false);               // Enable automatic RXM SFRBX messages 
  myGNSS.logRXMSFRBX();                              // Enable RXM SFRBX data logging
  myGNSS.setAutoRXMRAWX(true, false);                // Enable automatic RXM RAWX messages 
  myGNSS.logRXMRAWX();                               // Enable RXM RAWX data logging
}