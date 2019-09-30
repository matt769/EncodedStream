#ifndef COBS_H
#define COBS_H

#include<Arduino.h>

namespace COBS {
    void encode(const char buffer[], uint8_t size, char bufferEnc[]);
    void decode(const char bufferEnc[], uint8_t size, char buffer[]);
}

#endif
