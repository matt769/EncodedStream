#include "COBS.h"

namespace COBS {

  // when this function is called, the buffer will contain only the raw data
  // the size value will take into account only the raw data
  // it is allowed to send a zero length packet (size = 0)
  void encode(const char buffer[], uint8_t size, char bufferEnc[]) {
    uint8_t encIdx = 0;
  
    // first we put a leading zero in the encoded packet
    // this will hold the length of the first block of data (until we reach another 0) (which includes itself)
    bufferEnc[encIdx] = 0;
    // and record the position of this zero
    uint8_t lastZeroEncIdx = encIdx;
    encIdx++;
  
    // now copy the raw data in and encode as required
    for (uint8_t idx = 0; idx < size; idx++) {
      // if we've got another zero then need to 'close' the previous block
      // by replacing its leading zero with the length of the block
      if (buffer[idx] == 0) {
        bufferEnc[lastZeroEncIdx] = encIdx - lastZeroEncIdx;
        lastZeroEncIdx = encIdx;
      }
  
      // regardless, we still copy the new value in (if zero it will be modified later)
      bufferEnc[encIdx] = buffer[idx];
      encIdx++;
    }
  
    // close the last block
    bufferEnc[lastZeroEncIdx] = encIdx - lastZeroEncIdx;
    // and add a trailing zero to indicate the end of the packet
    bufferEnc[encIdx] = 0;
  
  }

  // size is the size of the encoded buffer, so includes 2 bytes in addition to the raw data
  void decode(const char bufferEnc[], uint8_t size, char buffer[]) {
  
    uint8_t blockSize = bufferEnc[0]; // the first byte will contain the length of the first block (but not be part of the decoded data itself)
    uint8_t currentSize = 1;
    bool newBlock = false;
  
    uint8_t idx = 0;
    for (uint8_t encIdx = 0; encIdx < size; encIdx++) {
  
      if (currentSize > blockSize) {
        newBlock = true;
        blockSize = bufferEnc[encIdx];
        currentSize = 1;
      }
      currentSize++;
  
      if (encIdx > 0 && encIdx < size - 1) {
        // if this is a new block then need to change the leading byte to zero
        // otherwise just take it as it is
        if (newBlock) {
          buffer[idx] = 0;
          newBlock = false;
        }
        else {
          buffer[idx] = bufferEnc[encIdx];
        }
  
        idx++;
      }
    }
  }


}
