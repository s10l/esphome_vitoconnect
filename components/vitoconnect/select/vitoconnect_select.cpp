#include "vitoconnect_select.h"
#include "esphome/core/log.h"

namespace esphome {
namespace vitoconnect {

static const char *TAG = "vitoconnect.select";

OPTOLINKSelect::OPTOLINKSelect() {
  // empty
}

OPTOLINKSelect::~OPTOLINKSelect() {
  // empty
}

void OPTOLINKSelect::set_option_labels(const std::vector<std::string> &labels) {
  this->option_labels_ = labels;
}

void OPTOLINKSelect::set_option_values(const std::vector<uint8_t> &values) {
  this->option_values_ = values;
}

void OPTOLINKSelect::control(const std::string &value) {
  ESP_LOGD(TAG, "Selected: %s", value.c_str());

  // find index for label
  for (size_t i = 0; i < this->option_labels_.size(); ++i) {
    if (this->option_labels_[i] == value) {
      this->current_value_ = (i < this->option_values_.size()) ? this->option_values_[i] : 0;
      this->_last_update = millis();
      publish_state(value);
      return;
    }
  }

  // fallback: publish as-is if label not found
  this->_last_update = millis();
  ESP_LOGW(TAG, "Label %s not found in options", value.c_str());
  publish_state(value);
}

void OPTOLINKSelect::decode(uint8_t* data, uint8_t length, Datapoint* dp) {
  assert(length >= _length);

  uint8_t value = 0;

  if (_length == 1) {
    value = (uint8_t) data[0];
  } else {
    ESP_LOGW(TAG, "Unsupported length %d", _length);
    return;
  }

  this->current_value_ = value;
  
  // find matching label
  for (size_t i = 0; i < this->option_values_.size(); ++i) {
    if (this->option_values_[i] == value) {
      publish_state(this->option_labels_[i]);
      return;
    }
  }
  
  // not found: publish numeric string
  ESP_LOGW(TAG, "Value %d has no matching label", value);
  publish_state(std::to_string(value));
}

void OPTOLINKSelect::encode(uint8_t* raw, uint8_t length) {
  uint8_t value = this->current_value_;
  encode(raw, length, &value);
}

void OPTOLINKSelect::encode(uint8_t* raw, uint8_t length, void* data) {
  uint8_t value = *reinterpret_cast<uint8_t*>(data);
  encode(raw, length, value);
}

void OPTOLINKSelect::encode(uint8_t* raw, uint8_t length, uint8_t data) {
  assert(length >= _length);
  uint8_t value = data;
  
  ESP_LOGD(TAG, "encode called with data: %d", value);
  
  if( _length == 1) {
    raw[0] = value;
  } else {
    ESP_LOGW(TAG, "Unsupported length %d", _length);
    return;
  }
}

}  // namespace vitoconnect
}  // namespace esphome
