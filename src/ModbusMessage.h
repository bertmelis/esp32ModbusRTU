/* ModbusMessage

Copyright 2018 Bert Melis

Permission is hereby granted, free of charge, to any person obtaining a
copy of this software and associated documentation files (the
"Software"), to deal in the Software without restriction, including
without limitation the rights to use, copy, modify, merge, publish,
distribute, sublicense, and/or sell copies of the Software, and to
permit persons to whom the Software is furnished to do so, subject to
the following conditions:

The above copyright notice and this permission notice shall be included
in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#ifndef esp32ModbusRTUInternals_ModbusMessage_h
#define esp32ModbusRTUInternals_ModbusMessage_h

#include <stdint.h>  // for uint*_t
#include <stddef.h>  // for size_t

#include "esp32ModbusTypeDefs.h"

namespace esp32ModbusRTUInternals {

class ModbusMessage {
 public:
  virtual ~ModbusMessage();
  uint8_t* getMessage();
  uint8_t getSize();
  void add(uint8_t value);

 protected:
  explicit ModbusMessage(uint8_t length);
  uint8_t* _buffer;
  uint8_t _length;
  uint8_t _index;
};

class ModbusResponse;  // forward declare for use in ModbusRequest

class ModbusRequest : public ModbusMessage {
 public:
  virtual size_t responseLength() = 0;

 protected:
  explicit ModbusRequest(uint8_t length);
  uint8_t _slaveAddress;
  uint8_t _functionCode;
  uint16_t _address;
  uint16_t _byteCount;
};

// read discrete coils
class ModbusRequest02 : public ModbusRequest {
 public:
  explicit ModbusRequest02(uint8_t slaveAddress, uint16_t address, uint16_t numberCoils);
  size_t responseLength();
};

// read holding registers
class ModbusRequest03 : public ModbusRequest {
 public:
  explicit ModbusRequest03(uint8_t slaveAddress, uint16_t address, uint16_t numberRegisters);
  size_t responseLength();
};

// read input registers
class ModbusRequest04 : public ModbusRequest {
 public:
  explicit ModbusRequest04(uint8_t slaveAddress, uint16_t address, uint16_t numberRegisters);
  size_t responseLength();
};

// write multiple holding registers
class ModbusRequest16 : public ModbusRequest {
 public:
  explicit ModbusRequest16(uint8_t slaveAddress, uint16_t address, uint16_t numberRegisters, uint8_t* data);
  size_t responseLength();
};

class ModbusResponse : public ModbusMessage {
 public:
  explicit ModbusResponse(uint8_t length, ModbusRequest* request);
  bool isComplete();
  bool isSucces();
  bool checkCRC();
  esp32Modbus::Error getError() const;

  uint8_t getSlaveAddress();
  esp32Modbus::FunctionCode getFunctionCode();
  uint8_t* getData();
  uint8_t getByteCount();

 private:
  ModbusRequest* _request;
  esp32Modbus::Error _error;
};

}  // namespace esp32ModbusRTUInternals

#endif
