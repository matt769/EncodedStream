// This sketch send some packages periodically and also listens for packages
// Two Arduinos each running this sketch can talk to each other using the encoded stream communications
// In this example, there are 3 different package types used, with the type indicated by the first byte of the package

#include <EncodedStream.h>

// Create instance of EncodedStream, providing a pointer to the stream used (in this case Serial)
//    and a reference to the function called when a valid packet is received
EncodedStream<30> encodedSerial(&Serial, &callback);

// For clarity these namespaces are used to distinguish between the sent and received values, but it is not required
namespace PackageSentA {
uint8_t a = 0;
int8_t b = 0;
uint16_t c = 0;
int16_t d = 0;
uint32_t e = 0;
int32_t f = 0;
float g = 0;
}

namespace PackageSentB {
uint8_t a = 0;
int8_t b = 0;
uint16_t c = 0;
}

namespace PackageSentC {
uint32_t e = 0;
int32_t f = 0;
float g = 0;
}

namespace PackageReceivedA {
uint8_t a = 0;
int8_t b = 0;
uint16_t c = 0;
int16_t d = 0;
uint32_t e = 0;
int32_t f = 0;
float g = 0;
}

namespace PackageReceivedB {
uint8_t a = 0;
int8_t b = 0;
uint16_t c = 0;
}

namespace PackageReceivedC {
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
    uint8_t packageType = random(0, 3);
    switch (packageType) {
      case 0:
        PackageSentA::a = random(0, 126);
        PackageSentA::b = random(-60, 60);
        PackageSentA::c = random(0, 30000);
        PackageSentA::d = random(-15000, 15000);
        PackageSentA::e = random(0, 2000000000);
        PackageSentA::f = random(-1000000000, 1000000000);
        PackageSentA::g = (float)random(-1000000, 1000000) / 100000.0;

        encodedSerial.addToBuffer(packageType);  // so the receiver knows what package structure it should expect
        encodedSerial.addToBuffer<uint8_t>(PackageSentA::a);
        encodedSerial.addToBuffer<int8_t>(PackageSentA::b);
        encodedSerial.addToBuffer<uint16_t>(PackageSentA::c);
        encodedSerial.addToBuffer<int16_t>(PackageSentA::d);
        encodedSerial.addToBuffer<uint32_t>(PackageSentA::e);
        encodedSerial.addToBuffer<int32_t>(PackageSentA::f);
        encodedSerial.addToBuffer<float>(PackageSentA::g);

        break;

      case 1:
        PackageSentB::a = random(0, 126);
        PackageSentB::b = random(-60, 60);
        PackageSentB::c = random(0, 30000);

        encodedSerial.addToBuffer(packageType);
        encodedSerial.addToBuffer<uint8_t>(PackageSentB::a);
        encodedSerial.addToBuffer<int8_t>(PackageSentB::b);
        encodedSerial.addToBuffer<uint16_t>(PackageSentB::c);

        break;

      case 2:
        PackageSentC::e = random(0, 2000000000);
        PackageSentC::f = random(-1000000000, 1000000000);
        PackageSentC::g = (float)random(-1000000, 1000000) / 100000.0;

        encodedSerial.addToBuffer(packageType);
        encodedSerial.addToBuffer<uint32_t>(PackageSentC::e);
        encodedSerial.addToBuffer<int32_t>(PackageSentC::f);
        encodedSerial.addToBuffer<float>(PackageSentC::g);

        break;

      default:
        break;

    }

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

  // extract the first byte which we're expecting to indicate the package type
  uint8_t receivedPackageType = encodedSerial.extractFromBuffer<uint8_t>();

  // then extract the rest depending on the package type
  switch (receivedPackageType) {
    case 0:
      PackageReceivedA::a = encodedSerial.extractFromBuffer<uint8_t>();
      PackageReceivedA::b = encodedSerial.extractFromBuffer<int8_t>();
      PackageReceivedA::c = encodedSerial.extractFromBuffer<uint16_t>();
      PackageReceivedA::d = encodedSerial.extractFromBuffer<int16_t>();
      PackageReceivedA::e = encodedSerial.extractFromBuffer<uint32_t>();
      PackageReceivedA::f = encodedSerial.extractFromBuffer<int32_t>();
      PackageReceivedA::g = encodedSerial.extractFromBuffer<float>();
      break;

    case 1:
      PackageReceivedB::a = encodedSerial.extractFromBuffer<uint8_t>();
      PackageReceivedB::b = encodedSerial.extractFromBuffer<int8_t>();
      PackageReceivedB::c = encodedSerial.extractFromBuffer<uint16_t>();
      break;

    case 2:
      PackageReceivedC::e = encodedSerial.extractFromBuffer<uint32_t>();
      PackageReceivedC::f = encodedSerial.extractFromBuffer<int32_t>();
      PackageReceivedC::g = encodedSerial.extractFromBuffer<float>();
      break;

    default:
      break;

  }

  // Do something cool with all your data

  return true; // return value not used at the moment

}
