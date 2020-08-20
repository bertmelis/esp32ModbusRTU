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

#if defined ARDUINO_ARCH_ESP32

using namespace esp32ModbusRTUInternals;  // NOLINT

esp32ModbusRTU::esp32ModbusRTU(HardwareSerial* serial, int8_t rtsPin) :
  TimeOutValue(TIMEOUT_MS),
  _serial(serial),
  _lastMicros(0),
  _interval(0),
  _rtsPin(rtsPin),
  _task(nullptr),
  _queue(nullptr),
  _onData(nullptr),
  _onError(nullptr),
  _onDataToken(nullptr),
  _onErrorToken(nullptr)  {
    _queue = xQueueCreate(QUEUE_SIZE, sizeof(ModbusRequest*));
}

esp32ModbusRTU::~esp32ModbusRTU() {
  // TODO(bertmelis): kill task and cleanup
}

void esp32ModbusRTU::begin(int coreID /* = -1 */) {
  // If rtsPin is >=0, the RS485 adapter needs send/receive toggle
  if (_rtsPin >= 0) {
    pinMode(_rtsPin, OUTPUT);
    digitalWrite(_rtsPin, LOW);
  }
  xTaskCreatePinnedToCore((TaskFunction_t)&_handleConnection, "esp32ModbusRTU", 4096, this, 5, &_task, coreID >= 0 ? coreID : NULL);
  // silent interval is at least 3.5x character time
  _interval = 35000000UL / _serial->baudRate();  // 3.5 * 10 bits * 1000 µs * 1000 ms / baud

  // The following is okay for sending at any baud rate, but problematic at receiving with baud rates above 35000,
  // since the calculated interval will be below 1000µs!
  // f.i. 115200bd ==> interval=304µs
  if (_interval < 1000) _interval = 1000;  // minimum of 1msec interval
}

bool esp32ModbusRTU::readDiscreteInputs(uint8_t slaveAddress, uint16_t address, uint16_t numberCoils, uint32_t token) {
  ModbusRequest* request = new ModbusRequest02(slaveAddress, address, numberCoils, token);
  return _addToQueue(request);
}
bool esp32ModbusRTU::readHoldingRegisters(uint8_t slaveAddress, uint16_t address, uint16_t numberRegisters, uint32_t token) {
  ModbusRequest* request = new ModbusRequest03(slaveAddress, address, numberRegisters, token);
  return _addToQueue(request);
}

bool esp32ModbusRTU::readInputRegisters(uint8_t slaveAddress, uint16_t address, uint16_t numberRegisters, uint32_t token) {
  ModbusRequest* request = new ModbusRequest04(slaveAddress, address, numberRegisters, token);
  return _addToQueue(request);
}

bool esp32ModbusRTU::writeSingleHoldingRegister(uint8_t slaveAddress, uint16_t address, uint16_t data, uint32_t token) {
  ModbusRequest* request = new ModbusRequest06(slaveAddress, address, data, token);
  return _addToQueue(request);
}

bool esp32ModbusRTU::writeMultHoldingRegisters(uint8_t slaveAddress, uint16_t address, uint16_t numberRegisters, uint8_t* data, uint32_t token) {
  ModbusRequest* request = new ModbusRequest16(slaveAddress, address, numberRegisters, data, token);
  return _addToQueue(request);
}

bool esp32ModbusRTU::rawRequest(uint8_t slaveAddress, uint8_t functionCode, uint16_t dataLength, uint8_t* data, uint32_t token) {
  ModbusRequest* request = new ModbusRequestRaw(slaveAddress, functionCode, dataLength, data, token);
  return _addToQueue(request);
}

void esp32ModbusRTU::onData(esp32Modbus::MBRTUOnData handler) {
  _onData = handler;
}

void esp32ModbusRTU::onError(esp32Modbus::MBRTUOnError handler) {
  _onError = handler;
}

void esp32ModbusRTU::onDataToken(esp32Modbus::MBRTUOnDataToken handler) {
  _onDataToken = handler;
}

void esp32ModbusRTU::onErrorToken(esp32Modbus::MBRTUOnErrorToken handler) {
  _onErrorToken = handler;
}

bool esp32ModbusRTU::_addToQueue(ModbusRequest* request) {
  if (!request) {
    return false;
  } else if (xQueueSend(_queue, reinterpret_cast<void*>(&request), (TickType_t)0) != pdPASS) {
    delete request;
    return false;
  } else {
    return true;
  }
}

void esp32ModbusRTU::_handleConnection(esp32ModbusRTU* instance) {
  while (1) {
    ModbusRequest* request;
    if (pdTRUE == xQueueReceive(instance->_queue, &request, portMAX_DELAY)) {  // block and wait for queued item
      instance->_send(request->getMessage(), request->getSize());
      ModbusResponse* response = instance->_receive(request);
      if (response->isSucces()) {
        // if the non-token onData handler is set, call it
        if (instance->_onData)
          instance->_onData(
            response->getSlaveAddress(),
            response->getFunctionCode(),
            request->getAddress(),
            response->getData(),
            response->getByteCount());
        // else, if the token onData handler is set, call that
        else if (instance->_onDataToken)
          instance->_onDataToken(
            response->getSlaveAddress(),
            response->getFunctionCode(),
            request->getAddress(),
            response->getData(),
            response->getByteCount(),
            response->getToken());
      } else {
        // Same for error responses. non-token onError set?
        if (instance->_onError) instance->_onError(response->getError());
        // No, but token onError instead?
        else if (instance->_onErrorToken) instance->_onErrorToken(response->getError(), response->getToken());
      }
      delete request;  // object created in public methods
      delete response;  // object created in _receive()
    }
  }
}

void esp32ModbusRTU::_send(uint8_t* data, uint8_t length) {
  while (micros() - _lastMicros < _interval) delayMicroseconds(1);  // respect _interval
  // Toggle rtsPin, if necessary
  if (_rtsPin >= 0) digitalWrite(_rtsPin, HIGH);
  _serial->write(data, length);
  _serial->flush();
  // Toggle rtsPin, if necessary
  if (_rtsPin >= 0) digitalWrite(_rtsPin, LOW);
  _lastMicros = micros();
}

// Adjust timeout on MODBUS - some slaves require longer/allow for shorter times
void esp32ModbusRTU::setTimeOutValue(uint32_t tov) {
  if (tov) TimeOutValue = tov;
}

ModbusResponse* esp32ModbusRTU::_receive(ModbusRequest* request) {
  // Allocate initial buffer size
  const uint16_t BUFBLOCKSIZE(128);
  uint8_t *buffer = new uint8_t[BUFBLOCKSIZE];
  uint8_t bufferBlocks = 1;

  // Index into buffer
  register uint16_t bufferPtr = 0;

  // State machine states
  enum STATES : uint8_t { WAIT_INTERVAL=0, WAIT_DATA, IN_PACKET, DATA_READ, ERROR_EXIT, FINISHED };
  register STATES state = WAIT_INTERVAL;

  // Timeout tracker
  uint32_t TimeOut = millis();

  // Error code
  esp32Modbus::Error errorCode = esp32Modbus::SUCCES;

  // Return data object
  ModbusResponse* response = nullptr;

  while(state != FINISHED)
  {
    switch(state)
    {
    // WAIT_INTERVAL: spend the remainder of the bus quiet time waiting
    case WAIT_INTERVAL:
      // Time passed?
      if(micros() - _lastMicros >= _interval) {
        // Yes, proceed to reading data
        state = WAIT_DATA;
      }
      else {
        // No, wait a little longer
        delayMicroseconds(1);
      }
      break;
    // WAIT_DATA: await first data byte, but watch timeout
    case WAIT_DATA:
      if(_serial->available()) {
        state = IN_PACKET;
        _lastMicros = micros();
      }
      else if(millis() - TimeOut >= TimeOutValue) {
        errorCode = esp32Modbus::TIMEOUT;
        state = ERROR_EXIT;
      }
      break;
    // IN_PACKET: read data until a gap of at least _interval time passed without another byte arriving
    case IN_PACKET:
      // Data waiting and space left in buffer?
      while (bufferPtr < bufferBlocks * BUFBLOCKSIZE && _serial->available()) {
        // Yes. Catch the byte
        buffer[bufferPtr++] = _serial->read();
        // Rewind timer
        _lastMicros = micros();
      }
      // Buffer full?
      if(bufferPtr >= bufferBlocks * BUFBLOCKSIZE) {
        // Yes. Extend it by another block
        bufferBlocks++;
        uint8_t *temp = new uint8_t[bufferBlocks * BUFBLOCKSIZE];
        memcpy(temp, buffer, (bufferBlocks - 1) * BUFBLOCKSIZE);
        delete buffer;
        buffer = temp;
      }
      // Gap of at least _interval micro seconds passed without data?
      if(micros() - _lastMicros <= _interval) {
        state = DATA_READ;
      }
      break;
    // DATA_READ: successfully gathered some data. Prepare return object.
    case DATA_READ:
      // Allocate response object
      response = new ModbusResponse(bufferPtr, request);
      // Move gathered data into it
      response->setData(bufferPtr, buffer);
      state = FINISHED;
      break;
    // ERROR_EXIT: We had a timeout. Prepare error return object
    case ERROR_EXIT:
      response = new ModbusResponse(5, request);
      response->setErrorResponse(request->getSlaveAddress(), request->getFunctionCode(), errorCode);
      state = FINISHED;
      break;
    // FINISHED: we are done, keep the compiler happy by pseudo-treating it.
    case FINISHED:
      break;
    }
  }

  // Deallocate buffer
  delete buffer;
  _lastMicros = micros();

  return response;
}

#elif defined ESP32MODBUSRTU_TEST

#else

#pragma message "no suitable platform"

#endif
