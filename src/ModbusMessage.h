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
#include <cstring>  // for memcpy

#include "esp32ModbusTypeDefs.h"

namespace esp32ModbusRTUInternals {

class ModbusMessage {
 public:
  virtual ~ModbusMessage();
  uint8_t* getMessage();
  uint8_t getSize();
  uint32_t getToken();
  void add(uint8_t value);

 protected:
  explicit ModbusMessage(uint8_t length);
  uint8_t* _buffer;
  uint8_t _length;
  uint8_t _index;
  uint32_t _token;
};

class ModbusResponse;  // forward declare for use in ModbusRequest

class ModbusRequest : public ModbusMessage {
 public:
  uint16_t getAddress();
  uint8_t getFunctionCode();
  uint8_t getSlaveAddress();

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
  explicit ModbusRequest02(uint8_t slaveAddress, uint16_t address, uint16_t numberCoils, uint32_t token = 0);
};

// read holding registers
class ModbusRequest03 : public ModbusRequest {
 public:
  explicit ModbusRequest03(uint8_t slaveAddress, uint16_t address, uint16_t numberRegisters, uint32_t token = 0);
};

// read input registers
class ModbusRequest04 : public ModbusRequest {
 public:
  explicit ModbusRequest04(uint8_t slaveAddress, uint16_t address, uint16_t numberRegisters, uint32_t token = 0);
};

// write single holding registers
class ModbusRequest06 : public ModbusRequest {
 public:
  explicit ModbusRequest06(uint8_t slaveAddress, uint16_t address, uint16_t data, uint32_t token = 0);
};

// write multiple holding registers
class ModbusRequest16 : public ModbusRequest {
 public:
  explicit ModbusRequest16(uint8_t slaveAddress, uint16_t address, uint16_t numberRegisters, uint8_t* data, uint32_t token = 0);
};

// "raw" request based on a request packet obtained as is.
class ModbusRequestRaw : public ModbusRequest {
 public:
  explicit ModbusRequestRaw(uint8_t slaveAddress, uint8_t functionCode, uint16_t dataLength, uint8_t* data, uint32_t token=0);
};

class ModbusResponse : public ModbusMessage {
 public:
  explicit ModbusResponse(uint8_t length, ModbusRequest* request);
  bool isSucces();
  bool checkCRC();
  esp32Modbus::Error getError() const;

  uint8_t getSlaveAddress();
  uint8_t getFunctionCode();
  uint8_t* getData();
  uint8_t getByteCount();
  void setErrorResponse(uint8_t errorCode);
  void setData(uint16_t dataLength, uint8_t *data);

 private:
  ModbusRequest* _request;
  esp32Modbus::Error _error;
};

}  // namespace esp32ModbusRTUInternals

#endif
