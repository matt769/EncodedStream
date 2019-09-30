#ifndef COBS_H
#define COBS_H

#ifdef __linux__
#include <stdint.h> // for fixed size data types
#else
#include<Arduino.h>
#endif


namespace COBS {
    void encode(const char buffer[], uint8_t size, char bufferEnc[]);
    void decode(const char bufferEnc[], uint8_t size, char buffer[]);
}

#endif
