# esp32ModbusRTU

[![Build Status](https://travis-ci.com/bertmelis/esp32ModbusRTU.svg?branch=master)](https://travis-ci.com/bertmelis/esp32ModbusRTU)

This is a non blocking Modbus client (master) for ESP32.

- Modbus Client aka Master for ESP32
- built for the [Arduino framework for ESP32](https://github.com/espressif/arduino-esp32)
- non blocking API. Blocking code is in a seperate task
- only RS485 half duplex using a GPIO as RTS (DE/RS) is implemented
- function codes implemented:
  - read discrete inputs (02)
  - read holding registers (03)
  - read input registers (04)
- similar API as my [esp32ModbusTCP](https://github.com/bertmelis/esp32ModbusTCP) implementation

## Developement status

I have this library in use myself with quite some uptime (only using FC3 -read holding registers- though).

Things to do, ranked:

- add debug info
- unit testing for ModbusMessage
- implement missing function codes (no priority, pull requests happily accepted)

## Installation

The easiest, and my preferred one, method to install this library is to use Platformio.
[https://platformio.org/lib/show/5902/esp32ModbusTCP/installation](https://platformio.org/lib/show/5902/esp32ModbusTCP/installation)

Alternatively, you can use Arduino IDE for developement.
[https://www.arduino.cc/en/guide/libraries](https://www.arduino.cc/en/guide/libraries)

Arduino framework for ESP32 v1.0.1 (January 2019) or higher is needed. Previous versions contain a bug where the last byte of the messages are truncated when using DE/RE
Arduino framework for ESP32 ([https://github.com/espressif/arduino-esp32](https://github.com/espressif/arduino-esp32))

## Example hardware:

```ASCII
   3.3V --------------+-----/\/\/\/\---+
                      |       680      |
              +-------x-------+        |
    17 <------| RO            |        |
              |              B|--------+-------------------------
    16 --+--->| DE  MAX3485   |        |                   \  /
         |    |               |        +-/\/\/\/\-+    RS-485 side
         +--->| /RE           |             120   |        /  \
              |              A|-------------------+---------------
     4 -------| DI            |                   |
              +-------x-------+                   |
                      |                           |
                      +-----/\/\/\/\--------------+
                      |       680
                      +----------------/\/\/\/\------------------ GND
                      |                  100
                     ---
```

The biasing resistors may not be neccesary for your setup. The GND connection
is connected via a 100 Ohms resistor to limit possible ground loop currents.

## Usage

The API is quite lightweight. It takes minimal 3 steps to get going.

First create the ModbusRTU object. The constructor takes two arguments: HardwareSerial object and pin number of DE/RS.

```C++
esp32ModbusRTU myModbus(&Serial, DE_PIN);
```

Next add a onData callback. This can be any function object. Here it uses a simple function pointer.

```C++

void handleData(uint8_t serverAddress, esp32Modbus::FunctionCode fc, uint8_t* data, size_t length) {
  Serial.printf("received data: id: %u, fc %u\ndata: 0x", serverAddress, fc);
  for (uint8_t i = 0; i < length; ++i) {
    Serial.printf("%02x", data[i]);
  }
  Serial.print("\n");
}

// in setup()
myModbus.onData(handleData);
```

Optionally you can attach an onError callback. Again, any function object is possible.

```C++
// in setup()
myModbus.onError([](esp32Modbus::Error error) {
  Serial.printf("error: 0x%02x\n\n", static_cast<uint8_t>(error));
});
```

After setup, start the modbusmaster:

```C++
// in setup()
myModbus.begin();
```

Now ModbusRTU is setup, you can start reading or writing. The arguments depend on the function used. Obviously, serverID, address and length are always required. The length is specified in words, not in bytes!

```C++
myModbus.readInputRegisters(0x01, 52, 2);  // serverId, address + length
```

The requests are places in a queue. The function returns immediately and doesn't wait for the server to respond.
Communication methods return a boolean value so you can check if the command was successful.

## Configuration

The request queue holds maximum 20 items. So a 21st request will fail until the queue has an empty spot. You can change the queue size in the header file or by using a compiler flag:

```C++
#define QUEUE_SIZE 20
```

The waiting time before a timeout error is returned can also be changed by a `#define` variable:

```C++
#define TIMEOUT_MS 5000
```

## Issues

Please file a Github issue ~~if~~ when you find a bug. You can also use the issue tracker for feature requests.

# Extra

For modbus-TCP, check out [esp32ModbusTCP](https://github.com/bertmelis/esp32ModbusTCP)
