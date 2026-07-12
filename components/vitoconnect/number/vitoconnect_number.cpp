#include "vitoconnect_number.h"
#include "../vitoconnect.h"
#include "esphome/core/log.h"
#include <cmath>

namespace esphome {
namespace vitoconnect {

static const char *TAG = "vitoconnect_number";

void OPTOLINKNumber::control(float value) {
  if (this->parent_ == nullptr) {
    ESP_LOGW(TAG, "No VitoConnect parent set for datapoint 0x%04X", this->getAddress());
    return;
  }
  if (!std::isfinite(value)) {
    ESP_LOGW(TAG, "Rejecting non-finite write for datapoint 0x%04X", this->getAddress());
    return;
  }

  const float min_value = this->traits.get_min_value();
  const float max_value = this->traits.get_max_value();
  if (!std::isnan(min_value) && value < min_value) {
    ESP_LOGW(TAG, "Rejecting write %.3f below min %.3f for datapoint 0x%04X", value, min_value, this->getAddress());
    return;
  }
  if (!std::isnan(max_value) && value > max_value) {
    ESP_LOGW(TAG, "Rejecting write %.3f above max %.3f for datapoint 0x%04X", value, max_value, this->getAddress());
    return;
  }

  ESP_LOGD(TAG, "Writing %.3f to datapoint 0x%04X", value, this->getAddress());
  if (!this->parent_->write_datapoint(this, &value)) {
    ESP_LOGW(TAG, "Failed to queue write for datapoint 0x%04X", this->getAddress());
  }
}

void OPTOLINKNumber::decode(uint8_t *data, uint8_t length, Datapoint *dp) {
  assert(length >= this->_length);

  if (this->_length == 1) {
    this->publish_state(static_cast<float>(data[0]));
  } else if (this->_length == 2) {
    int16_t tmp = data[1] << 8 | data[0];
    this->publish_state(tmp / 1.0f);
  } else if (this->_length == 4) {
    uint32_t tmp = data[3] << 24 | data[2] << 16 | data[1] << 8 | data[0];
    this->publish_state(tmp / 1.0f);
  }
}

void OPTOLINKNumber::encode(uint8_t *raw, uint8_t length, void *data) {
  float value = *reinterpret_cast<float *>(data);
  this->encode(raw, length, value);
}

void OPTOLINKNumber::encode(uint8_t *raw, uint8_t length, float data) {
  assert(length >= this->_length);
  memset(raw, 0, length);

  if (this->_length == 1) {
    raw[0] = static_cast<uint8_t>(std::floor(data + 0.5f));
  } else if (this->_length == 2) {
    int16_t tmp = static_cast<int16_t>(std::floor(data + 0.5f));
    raw[1] = tmp >> 8;
    raw[0] = tmp & 0xFF;
  } else if (this->_length == 4) {
    uint32_t tmp = static_cast<uint32_t>(std::floor(data + 0.5f));
    raw[3] = tmp >> 24;
    raw[2] = tmp >> 16;
    raw[1] = tmp >> 8;
    raw[0] = tmp & 0xFF;
  }
}

}  // namespace vitoconnect
}  // namespace esphome
