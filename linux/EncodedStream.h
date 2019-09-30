#ifndef ENCODED_STREAM_H
#define ENCODED_STREAM_H


#ifdef __linux__
#include <stdint.h> // for fixed size data types
#include <cstring> // for memcpy
#include <unistd.h> // for read/write

#else
#include<Arduino.h>

#endif


#include "COBS.h"

// this class just expects packets to be delimited by 0
// it doesn't know anything else about the structure
//  that is all defined in the callback function, and in the main program where packages are built

static_assert(sizeof(float) == 4, "float variable not expected size."); // necessary?
static_assert(sizeof(char) == 1, "char variable not expected size.");




template<uint8_t BUFFER_SIZE>
class EncodedStream {
#ifdef __linux__
    using ParsePackageCallback = bool(*)(EncodedStream<BUFFER_SIZE>& encodedStream);
#else
    using ParsePackageCallback = bool(*)(void);
#endif


  public:
    const uint8_t bufferSize;
#ifdef __linux__
    EncodedStream(int fileDescriptor, ParsePackageCallback parsePackageCallback);
#else
    EncodedStream(Stream* stream);
    EncodedStream(Stream* stream, ParsePackageCallback parsePackageCallback);
#endif

    // will return true if a full packet has been received and decoded
    // expected to be called at least as fast as packets are arriving (i.e. every time in (quick) loop())
    bool receive(); // to be called periodically (i.e. every loop())
    void send(); // encode and send via serial
    uint8_t getSendBufferSize() const;
    uint8_t getReceiveBufferSize() const;
    template<class T>
    void addToBuffer(T var);
    template<class T>
    T extractFromBuffer();

  private:
#ifdef __linux__
    int fileDescriptor;
#else
    Stream* stream;
#endif

    ParsePackageCallback parsePackageCallback = nullptr;
    char sendBuffer[BUFFER_SIZE + 3]; // This will contain the unencoded data (with 3 bytes space for encoding etc)
    // The encoded data will only be kept temporarily and immediately sent
    uint8_t sendBufferSize = 0; // the length of valid data in the send buffer
    char receiveBuffer[BUFFER_SIZE + 3];    // this will contain the encoded data as it comes in
    // once the packet is complete, it will be decoded and will replace the encoded data in this buffer
    uint8_t receiveBufferSize = 0; // the length of actual data in the receive buffer
    char calculateCheckSum(char buffer[], uint8_t size) const;
    void addCheckSum(char buffer[], uint8_t size); // appends checksum byte to buffer
    bool validateCheckSum(char buffer[], uint8_t size) const; // checks the last byte in the unencoded receive buffer
    char* extractPosition = receiveBuffer; // used to track progress of unpacking
    bool isBigEndian = true;
    static bool checkIfBigEndian();
    static void reverseByteOrder2(char buffer[]);
    static void reverseByteOrder4(char buffer[]);

};




/*************************************************************************************
 ********************************** DEFINITIONS **************************************
 *************************************************************************************
*/



#ifdef __linux__
template<uint8_t BUFFER_SIZE>
EncodedStream<BUFFER_SIZE>::EncodedStream(int fileDescriptor, ParsePackageCallback parsePackageCallback):

        bufferSize(BUFFER_SIZE + 3), // 3 bytes overhead
        fileDescriptor(fileDescriptor),
        parsePackageCallback(parsePackageCallback)
{
    static_assert(BUFFER_SIZE >= 0 && BUFFER_SIZE <= 252 , "Buffer size must be >=0 and <= 252.");
    isBigEndian = checkIfBigEndian();
}
#else
template<uint8_t BUFFER_SIZE>
EncodedStream<BUFFER_SIZE>::EncodedStream(Stream* stream, ParsePackageCallback parsePackageCallback):
  bufferSize(BUFFER_SIZE + 3), // 3 bytes overhead
  stream(stream),
  parsePackageCallback(parsePackageCallback)
{
  static_assert(BUFFER_SIZE >= 0 && BUFFER_SIZE <= 252 , "Buffer size must be >=0 and <= 252.");
  isBigEndian = checkIfBigEndian();
}
#endif


// initial state will be receiving because there is no start marker, only an end marker
// stop receiving when receive a 0
template<uint8_t BUFFER_SIZE>
bool EncodedStream<BUFFER_SIZE>::receive() {
  static bool bufferOverflow = false;
  static char c; // why did I make this static?
  static uint8_t idx = 0;
  static bool receiptComplete = false;

  // if receipt was previously complete, next time we're in this funtion set it back to false
  if (receiptComplete) {
    receiptComplete = false;
  }

#ifdef __linux__
    while ((read(fileDescriptor, &c, 1) == 1) && !receiptComplete) {
#else
    while ((stream->available() > 0) && !receiptComplete) {
    c = stream->read();
#endif

    receiveBuffer[idx] = c;
    if (c == 0) {
      // end of package
      receiptComplete = true;
      receiveBufferSize = idx + 1;
      idx = 0;
    }
    else if (idx < bufferSize - 1) {
      idx++;
    }
    else {
      bufferOverflow = true;
      // we've exceeded the buffer, prevent out of bounds access by no longer incrementing idx
    }
  }

  bool packetOk = false;
  if (receiptComplete) {
    if (!bufferOverflow) {
      char receiveBufferDec[bufferSize]; // have to create full size even if won't use it
      COBS::decode(receiveBuffer, receiveBufferSize, receiveBufferDec);

      // there will still be bytes leftover in the receiveBuffer but we reduce the receiveBufferSize so they won't be accessed
      receiveBufferSize -= 2; // don't need start/end bytes anymore
      for (uint8_t i = 0; i < receiveBufferSize; i++) {
        receiveBuffer[i] = receiveBufferDec[i];
      }

      packetOk = validateCheckSum(receiveBuffer, receiveBufferSize);
      // call user function if packet passed checks
      if (packetOk) {
        if (parsePackageCallback != nullptr) {
          receiveBufferSize--; // don't need checksum anymore
          extractPosition = receiveBuffer; // reset back to start of buffer before variable extraction happens in the callback

#ifdef __linux__
            parsePackageCallback(*this);
#else
            parsePackageCallback();
#endif

        }
      }
    }
    else {
      bufferOverflow = false;
    }
  }

  return receiptComplete && packetOk;
}

template<uint8_t BUFFER_SIZE>
void EncodedStream<BUFFER_SIZE>::send() {

  addCheckSum(sendBuffer, sendBufferSize);
  sendBufferSize++;

  char sendBufferEnc[bufferSize]; // have to create full size even if won't use it
  COBS::encode(sendBuffer, sendBufferSize, sendBufferEnc);

#ifdef __linux__
    write(fileDescriptor, sendBufferEnc, sendBufferSize + 2);
#else
    stream->write(sendBufferEnc, sendBufferSize + 2);
#endif

  sendBufferSize = 0;
}

template<uint8_t BUFFER_SIZE>
uint8_t EncodedStream<BUFFER_SIZE>::getSendBufferSize() const {
  return sendBufferSize;
}

template<uint8_t BUFFER_SIZE>
uint8_t EncodedStream<BUFFER_SIZE>::getReceiveBufferSize() const {
  return receiveBufferSize;
}

// Ideally want to only allow specific data types
template<uint8_t BUFFER_SIZE>
template<class T>
void EncodedStream<BUFFER_SIZE>::addToBuffer(T var) {
  char* endOfBuffer = sendBuffer + sendBufferSize;
  memcpy(endOfBuffer, &var, sizeof(var));
  if (isBigEndian) {
    if (sizeof(var) == 2) {
      reverseByteOrder2(endOfBuffer);
    }
    else if (sizeof(var) == 4) {
      reverseByteOrder4(endOfBuffer);
    }
  }
  sendBufferSize += sizeof(var);
}


template<uint8_t BUFFER_SIZE>
template<class T>
T EncodedStream<BUFFER_SIZE>::extractFromBuffer() {
  if (isBigEndian) {
    if (sizeof(T) == 2) {
      reverseByteOrder2(extractPosition);
    }
    else if (sizeof(T) == 4) {
      reverseByteOrder4(extractPosition);
    }
  }
  T ret;
  memcpy(&ret, extractPosition, sizeof(ret));
  extractPosition += sizeof(ret);
  return ret;
}



template<uint8_t BUFFER_SIZE>
char EncodedStream<BUFFER_SIZE>::calculateCheckSum(char buffer[], const uint8_t size) const {
  uint8_t checkSum = 0;
  for (int i = 0; i < size; i++) {
    checkSum += buffer[i];
    //    Serial.println(checkSum);
  }
  return (char)checkSum;
}

template<uint8_t BUFFER_SIZE>
void EncodedStream<BUFFER_SIZE>::addCheckSum(char buffer[], const uint8_t size) {
  buffer[size] = calculateCheckSum(buffer, size);
}

template<uint8_t BUFFER_SIZE>
bool EncodedStream<BUFFER_SIZE>::validateCheckSum(char buffer[], const uint8_t size) const {
  char checkSum = calculateCheckSum(buffer, size - 1);
  return (checkSum == buffer[size - 1]);
}

template<uint8_t BUFFER_SIZE>
bool EncodedStream<BUFFER_SIZE>::checkIfBigEndian() {
  uint8_t array2bytes[2];
  uint16_t int2bytes = 0x0102;
  memcpy(array2bytes, &int2bytes, 2);
  return (array2bytes[0] == 0x01);
}

template<uint8_t BUFFER_SIZE>
void EncodedStream<BUFFER_SIZE>::reverseByteOrder2(char buffer[]) {
  char tmp = buffer[0];
  buffer[0] = buffer[1];
  buffer[1] = tmp;
}

template<uint8_t BUFFER_SIZE>
void EncodedStream<BUFFER_SIZE>::reverseByteOrder4(char buffer[]) {
  char tmp = buffer[0];
  buffer[0] = buffer[3];
  buffer[3] = tmp;
  tmp = buffer[1];
  buffer[1] = buffer[2];
  buffer[2] = tmp;
}





#endif
