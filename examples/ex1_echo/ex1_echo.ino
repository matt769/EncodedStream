// This example is paired with example sketch "ex1_send_and_receive_reply"

// This sketch will receive packages from the other Arduino
//    and will decode and unpack them, perform a simple operation on them,
//    then repack, encode and send back to the other device


#include <EncodedStream.h>

// Create instance of EncodedStream, providing a pointer to the stream used (in this case Serial)
//    and a reference to the function called when a valid packet is received
EncodedStream<30> encodedSerial(&Serial, &callback);

void setup() {
  Serial.begin(115200);  // for the encoded communications
  pinMode(13, OUTPUT);    // it's more fun if some lights turn on and off

}

// All this sketch has to do is listen/receive and take some action if it receives a valid packet
void loop() {
  encodedSerial.receive();
}

// This is the user defined function that will process a complete and checksum validated package
bool callback() {
  digitalWrite(13, !digitalRead(13));

  // in this test we're expecting to receive a packet with the following structure
  //    uint8_t   232
  //    int8_t    -57
  //    uint16_t  35963
  //    int16_t   -13510
  //    uint32_t  2288217862
  //    int32_t   -1529788728
  //    float     -7.245600

  // Use the extract* functions to unpack the package
  // This must be in EXACTLY the same order and type as the package was built with addToBuffer()
  // Also perform some basic operation before sending back so we can be sure all numbers were interpreted correctly
  uint8_t a = encodedSerial.extractFromBuffer<uint8_t>() * 2;
  int8_t b = encodedSerial.extractFromBuffer<int8_t>() * 2;
  uint16_t c = encodedSerial.extractFromBuffer<uint16_t>() * 2;
  int16_t d = encodedSerial.extractFromBuffer<int16_t>() * 2;
  uint32_t e = encodedSerial.extractFromBuffer<uint32_t>() * 2;
  int32_t f = encodedSerial.extractFromBuffer<int32_t>() * 2;
  float g = encodedSerial.extractFromBuffer<float>() * 2.0;


  // to build the package, just keep calling the addToBuffer() function
  // the receiver will be expecting a specific (but configurable) order, so make sure everything is added in that order
  encodedSerial.addToBuffer<uint8_t>(a);
  encodedSerial.addToBuffer<int8_t>(b);
  encodedSerial.addToBuffer<uint16_t>(c);
  encodedSerial.addToBuffer<int16_t>(d);
  encodedSerial.addToBuffer<uint32_t>(e);
  encodedSerial.addToBuffer<int32_t>(f);
  encodedSerial.addToBuffer<float>(g);

  // Send the package
  // This will add a checksum, encode the packet using COBS, and send it
  encodedSerial.send();

  return true; // return value not used at the moment

}
