/*

Copyright 2018 Bert Melis

This example reads 2 words (4 bytes) from address 27 of a server with id 1.
The ESP is connected to a max3485 with pins 17 (RX), 4 (TX) and 16 as RTS.

   3.3V --------------+
                      |
              +-------x-------+
    17 <------| RO            |
              |              B|---------------
    16 --+--->| DE  MAX3485   |    \  /
         |    |               |   RS-485 side
         +--->| /RE           |    /  \
              |              A|---------------
     4 -------| DI            |
              +-------x-------+
                      |
                     ---

*/

#include <Arduino.h>
#include <esp32ModbusRTU.h>

esp32ModbusRTU modbus(&Serial1, 16);  // use Serial1 and pin 16 as RTS

void setup() {
  Serial.begin(115200);  // Serial output
  Serial1.begin(9600, SERIAL_8N1, 17, 4, true);  // Modbus connection

  modbus.onData([](uint8_t serverAddress, MBFunctionCode fc, uint8_t* data, size_t length) {
    Serial.printf("id %u, fc %u: ", serverAddress, fc);
    if (length > 8) length = 8;
    for (size_t i = 0; i < length; ++i) {
      Serial.printf("%02x", data[i]);
    }
    Serial.print("\n");
  });
  modbus.onError([](MBError error){
    Serial.printf("error: %02x\n", static_cast<uint8_t>(error));
  });
  modbus.begin();

}

void loop() {
  static uint32_t lastMillis = 0;
  if (millis() - lastMillis > 30000) {
    lastMillis = millis();
    Serial.print("sending Modbus request...\n");
    modbus.request(0x01, READ_INPUT_REGISTER, 27, 2);
  }
}
