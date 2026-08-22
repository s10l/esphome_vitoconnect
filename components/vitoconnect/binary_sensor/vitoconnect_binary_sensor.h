#pragma once

#include "esphome/components/binary_sensor/binary_sensor.h"
#include "../vitoconnect_datapoint.h"

namespace esphome {
namespace vitoconnect {
  
class OPTOLINKBinarySensor : public binary_sensor::BinarySensor, public Datapoint {

  public:
    OPTOLINKBinarySensor();
    ~OPTOLINKBinarySensor();

    void decode(uint8_t* data, uint8_t length, Datapoint* dp = nullptr) override;
    void encode(uint8_t* raw, uint8_t length, void* data) override;
    void encode(uint8_t* raw, uint8_t length, float data);
    void setBitMask(uint8_t mask);

  private:
    uint8_t _bit_mask_{0x01};

};

}  // namespace vitoconnect
}  // namespace esphome