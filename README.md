## Intro
It is common to send serial data as its character representation i.e.
```c++
Serial.println(123);
```
will send 3 bytes containing the ASCII codes of the characters '1', '2' and '3'.

Using this character representation:
* Easy to view data and debug using Arduino Serial monitor
* Flexibility in creating different packet structures
* Well suited for sending actual character data e.g. debug messages

But
* Inefficient for sending numerical information (123 only needs 1 byte to represent it)
* Requires converting numbers to strings or character arrays (takes time and may introduce rounding errors for floating point numbers)
* If the number needs to be used by a receiving program, need to reconstruct it

Using the unaltered binary representation:
* Transmission size kept to a minimum
* (Potentially) simple to reconstruct the numbers as no manipulation of the data is needed


There are various different ways to handle framing errors, package delimiters, having a package delimiter in the actual data etc etc. This article (https://www.embeddedrelated.com/showarticle/113.php) has a nice summary, including a description of its main focus, consistent overhead byte stuffing (COBS - http://conferences.sigcomm.org/sigcomm/1997/papers/p062.pdf). COBS encodes any data as a series of variable length blocks, preceded by the length of the block. A new block is only started when the packet delimiter byte is encountered in the data, and in this way is removed from the transmitted data. The overhead comes from an added first byte used to hold the length of the first block. For more information see the aforementioned article and paper.

Note that for very small packets, COBS may not be the most efficient encoding scheme.

## What does this library do?
* This library takes a (customisable) package structure and transmits it in COBS encoded format. A checksum is also added before encoding help detect transmission integrity (but cannot guarantee it). The total overhead for each packet is 3 bytes, 1 byte for COBS overhead plus 1 checksum byte plus 1 package delimiter byte.
* It can be used with any of the Arduino hardware or software serial instances.
* It provides functions to build a package easily, and functions to unpack a received package.
* Although COBS can be used for packages larger than 255 bytes, in this implementation the maximum is 252.



## How to use it
Include the header
```c++
#include <EncodedStream.h>
```

Instantiate an instance of the class with a pointer to the Stream to be used, and a pointer to the callback function that will process received packets (more information on this further down). The number given in **<>** is the buffer size, in this example it is 30 (bytes). The maximum is 252 (and the minimum is 0 but that might not be particularly useful).
```c++
EncodedStream<30> encodedSerial(&Serial, &callback);
```
The Stream object still needs to be be set up in the normal way
```c++
void setup() { Serial.begin(115200); }
```

To send a message (this will probably occur in loop() or a separate function, but isn't explicitly shown here):
```c++
uint8_t a = 2;
int8_t b = -5;
uint16_t c = 5555;
int16_t d = -10000;
uint32_t e = 789000;
int32_t f = -987654321;
float g = 1234.56789;
encodedSerial.addToBuffer<uint8_t>(a);
encodedSerial.addToBuffer<int8_t>(b);
encodedSerial.addToBuffer<uint16_t>(c);
encodedSerial.addToBuffer<int16_t>(d);
encodedSerial.addToBuffer<uint32_t>(e);
encodedSerial.addToBuffer<int32_t>(f);
encodedSerial.addToBuffer<float>(g);
encodedSerial.send();
```
`send()` will clear the send buffer so we can just start adding stuff again in the same way afterwards for the next package. Note that the maximum package length should not exceed the buffer size specified earlier.  
Note that the function type specifier e.g. `<int32_t>` is not strictly required (as the correct version can be deduced from the argument type) but is advisable for clarity. Make sure the correct type is used. **Only the above data types are supported.** The library does not prevent you from using others but I advise against it. Here it is very clear what all our data sizes are (except float, but this should usually be 4 bytes and compilation will fail if not).

To receive a message, keep calling receive() every loop. This function will process all currently available data up to the next package delimiter (so it must be called by a device at least as fast as packages are being sent to that device).
```c++
void loop() { encodedSerial.receive(); }
```

To deal with received messages, use a callback function (that was passed to the EncodedStream object upon creation). The callback takes the form:
```c++
bool callback(void);
```
and would probably be used similarly to this:
```c++
bool callback() {
  uint8_t a = encodedSerial.extractFromBuffer<uint8_t>();
  int8_t b = encodedSerial.extractFromBuffer<int8_t>();
  uint16_t c = encodedSerial.extractFromBuffer<uint16_t>();
  int16_t d = encodedSerial.extractFromBuffer<int16_t>();
  uint32_t e = encodedSerial.extractFromBuffer<uint32_t>();
  int32_t f = encodedSerial.extractFromBuffer<int32_t>();
  float g = encodedSerial.extractFromBuffer<float>();
  // do something with these values
  return true;
}
```
Use the extractFromBuffer functions to unpack the package. This must be in EXACTLY the same order and type as the package was built with addToBuffer(), and the appropriate function type must be used. **Only the above data types are supported.** The type specifier e.g. ```<int32_t>``` **is** required (unlike adding).

The data must be extracted before receive is called again else it might start receiving a new packet and the receive buffer will no longer be valid to parse.
If you want to do something extra in loop() after message receipt, you can set a flag in the callback function and then check/process it in loop().
The variables here could be global variables if you want to access them from other functions e.g. loop().

## Examples
`ex1_send_and_receive_reply.ino` and `ex1_echo.ino` - a test to show if the communication is working. Requires two Arduinos, one with multiple Serial ports (although could in theory use software serial (not tested)).  
`send_and_receive` - two Arduinos each running this sketch can talk to each other using the encoded stream communications.  
`send_and_receive_multiple_packet_types` - demonstrates using multiple package structures.


## Be aware of potential issues
### Using this to communicate between different architectures...
E.g. an Arduino and a PC. It *should* be ok, but you should test first.

### ... or two devices using different floating point representations
If you're not sure, then test it.

### Only the following data types have been tested
```c++
uint8_t, int8_t, uint16_t, int16_t, uint32_t, int32_t, float
```
In theory you could use things like `int` which does not have a guaranteed size and as long as it were the same size on both sender and receiver, or the size difference were taken into account when adding to and extracting from the buffer at each end, it would probably be fine, but it's probably a bad idea.

### Requirement to call begin()
E.g. `Serial.begin()` - don't forget!

### Not prevented from using other data types
The code will compile if you use other data types than advised above for `addToBuffer()` and `extractFromBuffer()` functions, but it may not run as expected/desired.


## Other
### Why not use a union of a custom struct and a char array?
The code would still have to deal with endianness which could cause some complications.  
I'm unclear if there could there be issues with struct padding?  
Undefined behaviour technically (although explicitly supported by gcc)  


### Do I have to use the callback function?
No... check receive() value, if true access receiveBuffer, need to modify class to make those variables public, or add public functions to access them. The callback function still needs to be present though but can just be empty.

### Communication problems are harder to debug when not sending data as its character representation
Yes. Curse your human brain.

### How can I send strings or char arrays?
There is currently no direct support for this but it should be fairly straighforward to add it.
It might be easiest to send a size / char array pair?

### Ideally the template functions should be restricted to only the supported types
Agreed, but I'm not sure how to do it for Arduino.

### Does this use big or little endian for the transmission?
Although standard network protocol is to transmit as big-endian, this library sends the data in little endian order i.e. the least significant byte of each variable is sent first. This is because the majority of architectures I expect this to be used on are little endian, so this way the library does not need to reverse the byte order when sending/receiving. Given that the sender and receiver need to know the exact package layout anyway, and so will probably both be using this library, I do not see any disadvantage to this.


## Using the code on Linux
In the linux/ folder you can find a set of files with slight alterations to compile and run on Linux. Or, if you want a single set of files that support both, look at this branch: *single_set_of_source_files_for_linux_and_arduino*. I didn't include these in the master branch because I wanted to keep the Arduino library code simpler.

See the Linux example `ex1_send_and_receive_reply.cpp` which is the equivalent of the Arduino sketch of the same name. The main change (in the library and the example) is that the on-packet-receipt callback function now takes a reference to the encoded stream instance, reflecting the fact that a C++ application on a normal computer is less likely to use global variables i.e. the encoded stream instance can be declared in main() instead. However, note that additional `template` keywords are required when calling the extract/add functions using such a reference (again, see the example).


## TODO
1. Check/prevention extracting data beyond the valid received data
2. Check/prevent adding data to buffer if it is going to exceed the buffer size