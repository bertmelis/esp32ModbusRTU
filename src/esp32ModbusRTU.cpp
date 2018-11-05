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

#include "esp32ModbusRTU.h"
#include "Helpers.h"
extern "C" {
  #include "crc.h"
}

esp32ModbusRTU::esp32ModbusRTU(HardwareSerial* serial, int8_t rtsPin) :
  _serial(serial),
  _rtsPin(rtsPin),
  _task(nullptr),
  _queue(nullptr) {
    _queue = xQueueCreate(QUEUE_SIZE, sizeof(MB_PDU));
}

esp32ModbusRTU::~esp32ModbusRTU() {
  // TODO(bertmelis): kill task and cleanup
}

void esp32ModbusRTU::begin() {
  if (_rtsPin < 0) {
    abort();
  }
  pinMode(_rtsPin, OUTPUT);
  digitalWrite(_rtsPin, LOW);
  xTaskCreate((TaskFunction_t)&_handleConnection, "esp32ModbusRTU", 4096, this, 5, &_task);
}

bool esp32ModbusRTU::request(uint8_t serverAddress, MBFunctionCode fc, uint16_t addr, uint16_t len, uint8_t* val) {
  MB_PDU mb_pdu = {serverAddress, fc, addr, len, val};
  if (xQueueSend(_queue, reinterpret_cast<void*>(&mb_pdu), (TickType_t)0) != pdPASS) {
    return false;
  }
  return true;
}

void esp32ModbusRTU::onData(MBOnData handler) {
  _onData = handler;
}

void esp32ModbusRTU::onError(MBOnError handler) {
  _onError = handler;
}

void esp32ModbusRTU::_handleConnection(esp32ModbusRTU* instance) {
  while (1) {
    MB_PDU mb_pdu;
    static uint8_t index = 0;
    static uint32_t lastMillis = 0;
    // uint32_t baudRate = _serial->baudRate();
    // silent Time is at least 3.5x character time, @9600baud = 3.5 x 1000 / 9600 msec = 0.36msec. taking 1msec
    static uint32_t interval = 1;
    static uint8_t rxBuffer[256];
    static uint8_t txBuffer[256];
    memset(rxBuffer, 0, sizeof(rxBuffer));

    xQueueReceive(instance->_queue, &mb_pdu, portMAX_DELAY);  // wait for queued item
    txBuffer[0] = mb_pdu.serverAddress;
    txBuffer[1] = mb_pdu.functionCode;
    txBuffer[2] = esp32ModbusRTUInternals::high(mb_pdu.address);
    txBuffer[3] = esp32ModbusRTUInternals::low(mb_pdu.address);
    txBuffer[4] = esp32ModbusRTUInternals::high(mb_pdu.length);
    txBuffer[5] = esp32ModbusRTUInternals::low(mb_pdu.length);
    uint16_t crc = esp32ModbusRTUInternals::CRC16(txBuffer, 6);
    memcpy(&txBuffer[6], &crc, 2);

    while (instance->_serial->available()) instance->_serial->read();  // clear TX buffer
    while (millis() - lastMillis < interval) delay(1);  // only start after 3.5 char pause
    digitalWrite(instance->_rtsPin, HIGH);
    instance->_serial->write(txBuffer, 8);
    instance->_serial->flush();
    delay(3);  // TODO(bertmelis): base delay on baudrate
    digitalWrite(instance->_rtsPin, LOW);
    lastMillis = millis();

    // read data, return on timeout or on complete message
    index = 0;
    MBError error = SUCCES;
    while (true) {
      if (instance->_serial->available()) {
        rxBuffer[index++] = instance->_serial->read();
      }
      if (index == (5 + rxBuffer[2])) {
        // message complete
        lastMillis = millis();
        // TOTO(bertmelis): error checking
        // 1. CRC
        // 2. returned FC
        // 3. match with request
        if (error == SUCCES) {
          if (instance->_onData) instance->_onData(rxBuffer[0], static_cast<MBFunctionCode>(rxBuffer[1]), &rxBuffer[3], rxBuffer[2]);
        } else {
          if (instance->_onError) instance->_onError(error);
        }
        break;
      }
      if (millis() - lastMillis > TIMEOUT_MS) {
        if (instance->_onError) instance->_onError(TIMEOUT);
        break;
      }
    }
  }
}
