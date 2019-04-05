/* copyright 2019 Bert Melis */

#include <ModbusMessage.h>

#include "Includes/catch.hpp"
#include "Includes/CheckArray.h"

using Catch::Matchers::WithinAbs;

TEST_CASE("Read input registers", "[FC04]") {
  esp32ModbusRTUInternals::ModbusRequest* request = new esp32ModbusRTUInternals::ModbusRequest04(0x11, 0x0008, 0x0001);
  uint8_t stdMessage[] = {0x11, 0x04, 0x00, 0x08, 0x00, 0x01, 0xB2, 0x98};
  uint8_t stdResponse[] = {0x11, 0x04, 0x02, 0x00, 0x0A, 0xF8, 0xF4};
  uint8_t stdErrorResponse[] = {0x11, 0x84, 0x02, 0x00, 0x00};  // TODO(bertmelis): adjust CRC

  REQUIRE(request->getSize() == sizeof(stdMessage));
  REQUIRE_THAT(request->getMessage(), ByteArrayEqual(stdMessage, sizeof(stdMessage)));

  esp32ModbusRTUInternals::ModbusResponse* response = new esp32ModbusRTUInternals::ModbusResponse(request->responseLength(), request);

  SECTION("normal response") {
    for (uint8_t i = 0; i < sizeof(stdResponse); ++i) {
      response->add(stdResponse[i]);
    }
    CHECK(response->isSucces());
  }

  SECTION("error response") {
    for (uint8_t i = 0; i < sizeof(stdErrorResponse); ++i) {
      response->add(stdErrorResponse[i]);
    }
    CHECK_FALSE(response->isSucces());
    CHECK(response->getError() == esp32Modbus::ILLEGAL_DATA_ADDRESS);
  }

  delete request;
  delete response;
}
