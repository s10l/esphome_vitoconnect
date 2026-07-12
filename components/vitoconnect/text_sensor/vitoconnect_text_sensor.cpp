#include "vitoconnect_text_sensor.h"
#include "esphome/core/helpers.h"

namespace esphome {
namespace vitoconnect {

OPTOLINKTextSensor::OPTOLINKTextSensor() {
  // empty
}

OPTOLINKTextSensor::~OPTOLINKTextSensor() {
  // empty
}

void OPTOLINKTextSensor::decode(uint8_t *data, uint8_t length, Datapoint *dp) {
  assert(length >= _length);

  if (!dp)
    dp = this;

  uint16_t value = data[0];
  if (_length == 2) {
    value = data[1] << 8 | data[0];
  }

  auto match = this->value_map_.find(value);
  if (match != this->value_map_.end()) {
    this->publish_state(match->second);
    return;
  }

  if (!this->unknown_value_.empty()) {
    this->publish_state(this->unknown_value_);
    return;
  }

  this->publish_state(to_string(value));
}

void OPTOLINKTextSensor::encode(uint8_t *raw, uint8_t length, void *data) {
  assert(length >= _length);
}

}  // namespace vitoconnect
}  // namespace esphome
