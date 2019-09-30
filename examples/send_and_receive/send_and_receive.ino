// This sketch send some packages periodically and also listens for packages
// Two Arduinos each running this sketch can talk to each other using the encoded stream communications

#include <EncodedStream.h>

// Create instance of EncodedStream, providing a pointer to the stream used (in this case Serial)
//    and a reference to the function called when a valid packet is received
EncodedStream<30> encodedSerial(&Serial, &callback);

// For clarity these namespaces are used to distinguish between the sent and received values, but it is not required
// There is no requirement for all packages to be the same layout
// e.g. device 1 could send a short message requesting data and device 2 could send back a longer message with the required data in a different format
//    as long as the package format is known, it can be unpacked correctly

namespace PackageSent {
uint8_t a = 0;
int8_t b = 0;
uint16_t c = 0;
int16_t d = 0;
uint32_t e = 0;
int32_t f = 0;
float g = 0;
}

namespace PackageReceived {
uint8_t a = 0;
int8_t b = 0;
uint16_t c = 0;
int16_t d = 0;
uint32_t e = 0;
int32_t f = 0;
float g = 0;
}

uint32_t timeLastSent;
uint32_t sendInterval = 1000; // 1 second

void setup() {
  Serial.begin(115200);  // for the encoded communications
  pinMode(13, OUTPUT);    // it's more fun if some lights turn on and off
  randomSeed(1);          // for repeatable results, useful for testing but not required
  timeLastSent = millis();
}



void loop() {

  // if enough time has passed, send another message
  if (millis() - timeLastSent > sendInterval) {

    // send some random data
    PackageSent::a = random(0, 126);
    PackageSent::b = random(-60, 60);
    PackageSent::c = random(0, 30000);
    PackageSent::d = random(-15000, 15000);
    PackageSent::e = random(0, 2000000000);
    PackageSent::f = random(-1000000000, 1000000000);
    PackageSent::g = (float)random(-1000000, 1000000) / 100000.0;

    // to build the package, just keep calling the addToBuffer() function
    // the receiver will be expecting a specific (but configurable) order, so make sure everything is added in that order
    encodedSerial.addToBuffer<uint8_t>(PackageSent::a);
    encodedSerial.addToBuffer<int8_t>(PackageSent::b);
    encodedSerial.addToBuffer<uint16_t>(PackageSent::c);
    encodedSerial.addToBuffer<int16_t>(PackageSent::d);
    encodedSerial.addToBuffer<uint32_t>(PackageSent::e);
    encodedSerial.addToBuffer<int32_t>(PackageSent::f);
    encodedSerial.addToBuffer<float>(PackageSent::g);
    
    // Send the package
    // This will add a checksum, encode the packet using COBS, and send it
    encodedSerial.send();

    timeLastSent += sendInterval;
  }

  // call this function every loop
  // this will process all available data up to the next packet delimiter
  encodedSerial.receive();
}


// This is the user defined function that will process a complete and checksum validated package
bool callback() {
  digitalWrite(13, !digitalRead(13));

  // Use the extract* functions to unpack the package
  // This must be in EXACTLY the same order and type as the package was built with addToBuffer()
  PackageReceived::a = encodedSerial.extractFromBuffer<uint8_t>();
  PackageReceived::b = encodedSerial.extractFromBuffer<int8_t>();
  PackageReceived::c = encodedSerial.extractFromBuffer<uint16_t>();
  PackageReceived::d = encodedSerial.extractFromBuffer<int16_t>();
  PackageReceived::e = encodedSerial.extractFromBuffer<uint32_t>();
  PackageReceived::f = encodedSerial.extractFromBuffer<int32_t>();
  PackageReceived::g = encodedSerial.extractFromBuffer<float>();
  
  // Do something cool with all your data

  return true; // return value not used at the moment

}
