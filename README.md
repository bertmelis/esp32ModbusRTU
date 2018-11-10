# esp32ModbusRTU

[![Build Status](https://travis-ci.com/bertmelis/esp32ModbusRTU.svg?branch=master)](https://travis-ci.com/bertmelis/esp32ModbusRTU)

# WORK IN PROGRESS - Current status:

- it is working
- only RS485 half duplex with GPIO as RTS (DE/RS) pin. connections are implemented
- only function code 04 is implemented (others will be there soon)
- async operation. blocking code is in a seperate task
- error codes are implemented but untested (somehow my device just tries to answer all the time instead of generating a Modbus error)

To add new function codes:
- create class, inherit from `ModbusRequest`
- the constructor needs to get all the needed data as arguments
- in the constructor
  - call `ModbusRequest(size_t)` in the initializer list with the needed message length as argument
  - create the modbus request by sequentially call `add(uint8_t)` to add a byte to the message buffer
  (first byte first, so start with the slave address)
- add a (virtual) method:

  ```C++
  ModbusResponse* ModbusRequestType::makeResponse() {
    uint8_t responseLength = 3 + (_byteCount * 2) + 2;  // expected length in the response
    ModbusResponse* response = new ModbusResponse(responseLength, this);
    return response;
  }
  ```
Note: the response will be deleted elsewhere.

# Example hardware:

```
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

# Extra

For modbus-TCP, check out [esp32ModbusTCP](https://github.com/bertmelis/esp32ModbusTCP)
