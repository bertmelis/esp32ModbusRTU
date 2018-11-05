/* esp32ModbusRTU

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

#pragma once

#define QUEUE_SIZE 20
#define TIMEOUT_MS 10000

#include <functional>

extern "C" {
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/queue.h>
}
#include <HardwareSerial.h>
#include <esp32-hal-gpio.h>

enum MBFunctionCode : uint8_t {
  READ_COIL            = 0x01,
  READ_DISC_INPUT      = 0x02,
  READ_HOLD_REGISTER   = 0x03,
  READ_INPUT_REGISTER  = 0x04,
  WRITE_COIL           = 0x05,
  WRITE_HOLD_REGISTER  = 0x06,
  WRITE_MULT_COILS     = 0x0F,
  WRITE_MULT_REGISTERS = 0x10
};

enum MBError : uint8_t {
  SUCCES                = 0x00,
  ILLEGAL_FUNCTION      = 0x01,
  ILLEGAL_DATA_ADDRESS  = 0x02,
  ILLEGAL_DATA_VALUE    = 0x03,
  SERVER_DEVICE_FAILURE = 0x04,
  ACKNOWLEDGE           = 0x05,
  SERVER_DEVICE_BUSY    = 0x06,
  NEGATIVE_ACKNOWLEDGE  = 0x07,
  MEMORY_PARITY_ERROR   = 0x08,
  TIMEOUT               = 0xE0,
  INVALID_SLAVE         = 0xE1,
  INVALUD_FUNCTION      = 0xE2
};

struct MB_PDU {
  uint8_t serverAddress;
  MBFunctionCode functionCode;
  uint16_t address;
  uint16_t length;
  uint8_t* value;
};

typedef std::function<void(uint8_t, MBFunctionCode, uint8_t*, size_t)> MBOnData;
typedef std::function<void(MBError)> MBOnError;

class esp32ModbusRTU {
 public:
  esp32ModbusRTU(HardwareSerial* serial, int8_t rtsPin = -1);
  ~esp32ModbusRTU();
  void begin();
  bool request(uint8_t serverAddress, MBFunctionCode fc, uint16_t addr, uint16_t len, uint8_t* val = nullptr);
  void onData(MBOnData handler);
  void onError(MBOnError handler);

 private:
  HardwareSerial* _serial;
  int8_t _rtsPin;
  TaskHandle_t _task;
  QueueHandle_t _queue;
  static void _handleConnection(esp32ModbusRTU* instance);
  MBOnData _onData;
  MBOnError _onError;
};
