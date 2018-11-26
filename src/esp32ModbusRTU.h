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

#ifndef QUEUE_SIZE
#define QUEUE_SIZE 20
#endif
#ifndef TIMEOUT_MS
#define TIMEOUT_MS 5000
#endif

#include <functional>

extern "C" {
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/queue.h>
}
#include <HardwareSerial.h>
#include <esp32-hal-gpio.h>

#include "esp32ModbusTypeDefs.h"
#include "ModbusMessage.h"

using namespace esp32Modbus;  // NOLINT

class esp32ModbusRTU {
 public:
  explicit esp32ModbusRTU(HardwareSerial* serial, int8_t rtsPin = -1);
  ~esp32ModbusRTU();
  void begin();
  bool readInputRegister(uint8_t slaveAddress, uint16_t address, uint16_t byteCount);
  void onData(MBRTUOnData handler);
  void onError(MBOnError handler);

 private:
  static void _handleConnection(esp32ModbusRTU* instance);
  void _send(uint8_t* data, uint8_t length);
  ModbusResponse* _receive(ModbusRequest* request);

 private:
  HardwareSerial* _serial;
  uint32_t _lastMillis;
  uint32_t _interval;
  int8_t _rtsPin;
  TaskHandle_t _task;
  QueueHandle_t _queue;
  MBRTUOnData _onData;
  MBOnError _onError;
};
