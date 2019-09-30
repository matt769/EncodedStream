
#include <fcntl.h>
#include <termios.h>
#include<iostream>

#include "EncodedStream.h"


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

// maybe give callback a reference to encodedSerial?
// although it would then become a template function
// try this with EncodedStream_alt1.h
// This DOES WORK
// but unfortunately makes the syntax look awful!

template<uint8_t BUFFER_SIZE>
bool callback(EncodedStream<BUFFER_SIZE>& encodedStream) {
    // Use the extract* functions to unpack the package
    // This must be in EXACTLY the same order and type as the package was built with addToBuffer()

    PackageReceived::a = encodedStream.template extractFromBuffer<uint8_t>();
    PackageReceived::b = encodedStream.template extractFromBuffer<int8_t>();
    PackageReceived::c = encodedStream.template extractFromBuffer<uint16_t>();
    PackageReceived::d = encodedStream.template extractFromBuffer<int16_t>();
    PackageReceived::e = encodedStream.template extractFromBuffer<uint32_t>();
    PackageReceived::f = encodedStream.template extractFromBuffer<int32_t>();
    PackageReceived::g = encodedStream.template extractFromBuffer<float>();

    // View the received package on the Serial monitor
    std::cout << "Received package:\n";
    std::cout << (int)PackageReceived::a << "\t";
    std::cout << (int)PackageReceived::b << "\t";
    std::cout << PackageReceived::c << "\t";
    std::cout << PackageReceived::d << "\t";
    std::cout << PackageReceived::e << "\t";
    std::cout << PackageReceived::f << "\t";
    std::cout << PackageReceived::g << "\n";


    // Check if the received package is what we expected
    std::cout << "Match: ";
    if (PackageReceived::a == PackageSent::a * 2 &&
        PackageReceived::b == PackageSent::b * 2 &&
        PackageReceived::c == PackageSent::c * 2 &&
        PackageReceived::d == PackageSent::d * 2 &&
        PackageReceived::e == PackageSent::e * 2 &&
        PackageReceived::f == PackageSent::f * 2 &&
        PackageReceived::g == PackageSent::g * 2.0) {
        std::cout << "YES\n";
    }
    else {
        std::cout << "NO\n";
    }

    return true;
}


int main() {

    struct termios tio;
    int tty_fd;

    memset(&tio, 0, sizeof(tio));
    tio.c_iflag = 0; // Not using any of the c_iflag (input) options
    tio.c_oflag = 0; // Not using any of the (c_oflag) output options
    tio.c_cflag = CS8 |                 // 8 bit, no parity, 1 stop bit
                  CREAD |             // Enable receiver
                  CLOCAL;             // Ignore modem control lines (not sure if required)
    tio.c_lflag = 0;  // Not using any of the c_lflag options
    tio.c_cc[VMIN] = 1; // Minimum number of characters for noncanonical read (not sure if necessary given non-blocking option)
    tio.c_cc[VTIME] = 0; // Timeout in deciseconds for noncanonical read (not sure if necessary given non-blocking option)

    // TODO add check if open was successful (check errno?)
    tty_fd = open("/dev/ttyACM0",
                  O_RDWR | O_NONBLOCK); // open for read and write | non-blocking (read will return immediately)
    cfsetospeed(&tio, B115200);            // 115200 baud
    cfsetispeed(&tio, B115200);            // 115200 baud

    tcsetattr(tty_fd, TCSANOW, &tio); // Apply changes (immediately)


    // Should probably add some check that open() worked properly



    EncodedStream<30> encodedSerial(tty_fd, &callback);
    std::srand(0);

    for (int testNo = 0; testNo < 2; testNo++) {

        // send some random data
        // since this data will be multiplied by 2 by the receiver, restrict to half range (although overflow behaviour should be the same anyway)
        PackageSent::a = std::rand() % 128;       // 2147483647
        PackageSent::b = std::rand() % 128 - 63;
        PackageSent::c = std::rand() % 16000;
        PackageSent::d = std::rand() % 16000 - 8000;
        PackageSent::e = std::rand();
        PackageSent::f = std::rand() - RAND_MAX/2;
        PackageSent::g = (float)(rand() % 10000 - 5000) / 100000.0;

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
        std::cout << "Sent package:\n";
        std::cout << (int)PackageSent::a << "\t";
        std::cout << (int)PackageSent::b << "\t";
        std::cout << PackageSent::c << "\t";
        std::cout << PackageSent::d << "\t";
        std::cout << PackageSent::e << "\t";
        std::cout << PackageSent::f << "\t";
        std::cout << PackageSent::g << "\n";


        // Send the package
        // This will add a checksum, encode the packet using COBS, and send over Serial1
        encodedSerial.send();

        // short delay to ensure that we should have a FULL response available (if one if coming)
        // this is just for ease of testing so we can compare sent and received data, it is not required
        usleep(50000);

        // now try and receive (this will process all available data up to the next packet delimiter)
        encodedSerial.receive();

    }


//    uint8_t a = encodedSerial.extractFromBuffer<uint8_t>();
//    std::cout << a << "\n";
//
//    EncodedStream<30>* encodedSerialPtr = &encodedSerial;
//    uint8_t b = encodedSerialPtr->extractFromBuffer<uint8_t>();
//    std::cout << b << "\n";
//
//    EncodedStream<30>& encodedSerialRef = encodedSerial;
//    uint8_t c = encodedSerialRef.extractFromBuffer<uint8_t>();
//    std::cout << c << "\n";


    close(tty_fd);




}
