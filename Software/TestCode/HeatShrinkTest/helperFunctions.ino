#ifdef HEATSHRINK_DEBUG
static void dump_buf(const char *name, uint8_t *buf, uint16_t count) {
    for (int i=0; i<count; i++) {
        uint8_t c = (uint8_t)buf[i];
        //printf("%s %d: 0x%02x ('%c')\n", name, i, c, isprint(c) ? c : '.');
        Serial.print(name);
        Serial.print(F(" "));
        Serial.print(i);
        Serial.print(F(": 0x"));
        Serial.print(c, HEX);
        Serial.print(F(" ('"));
//        Serial.print(isprint(c) ? c : '.', BYTE);
        Serial.print(F("')\n"));
    }
}
#endif

static void compress(uint8_t *input,
                     uint32_t input_size,
                     uint8_t *output,
                     uint32_t &output_size
                    ){
    heatshrink_encoder_reset(&hse);
    // compress(orig_buffer,40,comp_buffer,comp_size);
//    #ifdef HEATSHRINK_DEBUG
//    Serial.print(F("\n^^ COMPRESSING\n"));
//    dump_buf("input", input, input_size);
//    #endif
        
    size_t   count  = 0;
    uint32_t sunk   = 0;
    uint32_t polled = 0;
    while (sunk < input_size) {
        heatshrink_encoder_sink(&hse, &input[sunk], input_size - sunk, &count);
        sunk += count;
        #ifdef HEATSHRINK_DEBUG
        Serial.print(F("^^ sunk "));
        Serial.print(count);
        Serial.print(F("\n"));
        #endif
        if (sunk == input_size) {
            heatshrink_encoder_finish(&hse);
        }

        HSE_poll_res pres;
        do {                    /* "turn the crank" */
            pres = heatshrink_encoder_poll(&hse,&output[polled], output_size - polled, &count);
            polled += count;
            #ifdef HEATSHRINK_DEBUG
            Serial.print(F("^^ polled "));
            Serial.print(polled);
            Serial.print(F("\n"));
            #endif
        } while (pres == HSER_POLL_MORE);
        #ifdef HEATSHRINK_DEBUG
        if (polled >= output_size){
            Serial.print(F("FAIL: Exceeded the size of the output buffer!\n"));
        }
        #endif
        if (sunk == input_size) {
            heatshrink_encoder_finish(&hse);
        }
    }
    #ifdef HEATSHRINK_DEBUG
    Serial.print(F("in: "));
    Serial.print(input_size);
    Serial.print(F(" compressed: "));
    Serial.print(polled);
    Serial.print(F(" \n"));
    #endif
    //update the output size to the (smaller) compressed size
    output_size = polled;
    
    #ifdef HEATSHRINK_DEBUG
    dump_buf("output", output, output_size);
    #endif
}
