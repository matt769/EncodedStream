// This example is paired with example sketch "ex1_echo"

// This sketch must be on a device with multiple Serial ports e.g. Arduino Mega
// This device will be connected to the host computer via the Serial port (for printing debugging info to the Arduino IDE Serial Monitor)
//  and to another Arduino compatible device on the Serial1 port

// This sketch will build some packages (according to a specific package layout) and send them to the other Arduino
//    which will decode and unpack them, perform a simple operation on them,
//    then repack, encode and send back to this device where they can be compared to the original packet sent



#include <EncodedStream.h>

// Create instance of EncodedStream, providing a pointer to the stream used (in this case Serial)
//    and a reference to the function called when a valid packet is received
EncodedStream<30> encodedSerial(&Serial1, &callback);

// For clarity these namespaces are used to distinguish between the sent and received values, but it is not required
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


void setup() {
  Serial.begin(115200);   // for debug information to the Serial monitor
  Serial1.begin(115200);  // for the encoded communications
  pinMode(13, OUTPUT);    // it's more fun if some lights turn on and off
  randomSeed(1);          // for repeatable results, useful for testing but not required

  for (int testNo = 0; testNo < 10; testNo++) {

    // send some random data
    // since this data will be multiplied by 2 by the receiver, restrict to half range (although overflow behaviour should be the same anyway)
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

    // View the created package on the Serial monitor
    Serial.print("Sent package:\n");
    Serial.print(PackageSent::a); Serial.print('\t');
    Serial.print(PackageSent::b); Serial.print('\t');
    Serial.print(PackageSent::c); Serial.print('\t');
    Serial.print(PackageSent::d); Serial.print('\t');
    Serial.print(PackageSent::e); Serial.print('\t');
    Serial.print(PackageSent::f); Serial.print('\t');
    Serial.print(PackageSent::g, 7); Serial.print('\n');

    // Send the package
    // This will add a checksum, encode the packet using COBS, and send over Serial1
    encodedSerial.send();


    // short delay to ensure that we should have a FULL response available (if one if coming)
    // this is just for ease of testing so we can compare sent and received data, it is not required
    delay(50);

    // now try and receive (this will process all available data up to the next packet delimiter)
    encodedSerial.receive();

  }
}



void loop() {}  // Empty loop, all tests done from setup()


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

  // View the received package on the Serial monitor
  Serial.print("Received package:\n");
  Serial.print(PackageReceived::a); Serial.print('\t');
  Serial.print(PackageReceived::b); Serial.print('\t');
  Serial.print(PackageReceived::c); Serial.print('\t');
  Serial.print(PackageReceived::d); Serial.print('\t');
  Serial.print(PackageReceived::e); Serial.print('\t');
  Serial.print(PackageReceived::f); Serial.print('\t');
  Serial.print(PackageReceived::g, 7); Serial.print('\n');

  // Check if the received package is what we expected
  Serial.print("Match: ");
  if (PackageReceived::a == PackageSent::a * 2 &&
      PackageReceived::b == PackageSent::b * 2 &&
      PackageReceived::c == PackageSent::c * 2 &&
      PackageReceived::d == PackageSent::d * 2 &&
      PackageReceived::e == PackageSent::e * 2 &&
      PackageReceived::f == PackageSent::f * 2 &&
      PackageReceived::g == PackageSent::g * 2.0) {
    Serial.print("YES\n");
  }
  else {
    Serial.print("NO\n");
  }


  return true; // return value not used at the moment

}
