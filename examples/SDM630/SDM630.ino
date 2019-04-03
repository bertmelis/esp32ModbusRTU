/*

Copyright 2018 Bert Melis

This example reads 2 words (4 bytes) from address 52 of a server with id 1.
address 52 = register 30053 (Eastron SDM630 Total system power)
The ESP is connected to a max3485 with pins 17 (RX), 4 (TX) and 16 as RTS.

*/

#include <Arduino.h>
#include <esp32ModbusRTU.h>
#include <algorithm>  // for std::reverse

esp32ModbusRTU modbus(&Serial1, 16);  // use Serial1 and pin 16 as RTS

void setup() {
  Serial.begin(115200);  // Serial output
  Serial1.begin(9600, SERIAL_8N1, 17, 4, true);  // Modbus connection

  modbus.onData([](uint8_t serverAddress, esp32Modbus::FunctionCode fc, uint8_t* data, size_t length) {
    Serial.printf("id 0x%02x fc 0x%02x len %u: 0x", serverAddress, fc, length);
    for (size_t i = 0; i < length; ++i) {
      Serial.printf("%02x", data[i]);
    }
    std::reverse(data, data + 4);  // fix endianness
    Serial.printf("\nval: %.2f", *reinterpret_cast<float*>(data));
    Serial.print("\n\n");
  });
  modbus.onError([](esp32Modbus::Error error) {
    Serial.printf("error: 0x%02x\n\n", static_cast<uint8_t>(error));
  });
  modbus.begin();

}

void loop() {
  static uint32_t lastMillis = 0;
  if (millis() - lastMillis > 30000) {
    lastMillis = millis();
    Serial.print("sending Modbus request...\n");
    modbus.readInputRegisters(0x01, 52, 2);
  }
}
