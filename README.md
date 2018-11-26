# esp32ModbusRTU

[![Build Status](https://travis-ci.com/bertmelis/esp32ModbusRTU.svg?branch=master)](https://travis-ci.com/bertmelis/esp32ModbusRTU)

# WORK IN PROGRESS - Current status:

- it is working
- only RS485 half duplex using a GPIO as RTS (DE/RS) is implemented
- function codes implemented:
  - read discrete inputs (02)
  - read holding registers (03)
  - read input registers (04)
- async operation. blocking code is in a seperate task
- error codes are implemented but untested (somehow my device just tries to answer all the time instead of generating a Modbus error)


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
