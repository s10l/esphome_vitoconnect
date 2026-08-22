#include "vitoconnect_select.h"
#include "esphome/core/log.h"

#include <cstdlib>

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

int OPTOLINKSelect::find_label_index_(const std::string &label) const {
  for (size_t i = 0; i < this->option_labels_.size(); ++i) {
    if (this->option_labels_[i] == label) return static_cast<int>(i);
  }
  return -1;
}

int OPTOLINKSelect::find_value_index_(uint8_t value) const {
  for (size_t i = 0; i < this->option_values_.size(); ++i) {
    if (this->option_values_[i] == value) return static_cast<int>(i);
  }
  return -1;
}

std::string OPTOLINKSelect::label_for_value_(uint8_t value) const {
  const int idx = this->find_value_index_(value);
  if (idx >= 0 && static_cast<size_t>(idx) < this->option_labels_.size()) {
    return this->option_labels_[idx];
  }
  return std::to_string(value);
}

bool OPTOLINKSelect::is_allowed_value_(uint8_t value) const {
  return this->find_value_index_(value) >= 0;
}

void OPTOLINKSelect::stage_value_(uint8_t value, const std::string &publish_value) {
  this->current_value_ = value;
  this->_command_value = value;
  this->_has_command_value = true;

  uint32_t seq = millis();
  if (seq == 0) seq = 1;
  this->_last_update = seq;

  publish_state(publish_value);
  ESP_LOGD(TAG, "Staged select %s -> raw=%u", this->get_name().c_str(), static_cast<unsigned>(value));
}

void OPTOLINKSelect::control(const std::string &value) {
  ESP_LOGD(TAG, "Selected %s: %s", this->get_name().c_str(), value.c_str());

  const int label_idx = this->find_label_index_(value);
  if (label_idx >= 0) {
    if (static_cast<size_t>(label_idx) >= this->option_values_.size()) {
      ESP_LOGW(TAG, "Label %s has no mapped raw value; refusing to write", value.c_str());
      return;
    }
    this->stage_value_(this->option_values_[label_idx], value);
    return;
  }

  // Home Assistant normally sends labels from the configured options. Accept a
  // numeric string as a guarded fallback, but only if the value is in options.
  char *end = nullptr;
  long parsed = strtol(value.c_str(), &end, 10);
  const bool is_int = (end != value.c_str()) && (*end == '\0');
  if (is_int && parsed >= 0 && parsed <= 255) {
    const uint8_t u = static_cast<uint8_t>(parsed);
    if (!this->is_allowed_value_(u)) {
      ESP_LOGW(TAG, "Numeric option %ld not in allowed option_values; refusing to write", parsed);
      return;
    }
    this->stage_value_(u, this->label_for_value_(u));
    return;
  }

  ESP_LOGW(TAG, "Label %s not found in options; refusing to write", value.c_str());
}

void OPTOLINKSelect::decode(uint8_t* data, uint8_t length, Datapoint* dp) {
  if (_length == 0) {
    ESP_LOGW(TAG, "Unsupported zero length for %s", this->get_name().c_str());
    return;
  }
  if (length < _length) {
    ESP_LOGW(TAG, "decode length mismatch for %s: got=%u expected=%u",
             this->get_name().c_str(), (unsigned) length, (unsigned) _length);
    return;
  }

  uint8_t value = 0;

  if (_length == 1) {
    value = (uint8_t) data[0];
  } else {
    ESP_LOGW(TAG, "Unsupported length %d", _length);
    return;
  }

  this->current_value_ = value;
  this->_last_read_ms = millis();

  if (this->_has_command_value && value == this->_command_value) {
    this->_has_command_value = false;
  }

  const std::string label = this->label_for_value_(value);
  if (this->find_value_index_(value) < 0) {
    ESP_LOGW(TAG, "Value %u has no matching label for %s", static_cast<unsigned>(value), this->get_name().c_str());
  }
  publish_state(label);
}

void OPTOLINKSelect::encode(uint8_t* raw, uint8_t length) {
  uint8_t value = this->_has_command_value ? this->_command_value : this->current_value_;
  encode(raw, length, &value);
}

void OPTOLINKSelect::encode(uint8_t* raw, uint8_t length, void* data) {
  if (data == nullptr) {
    ESP_LOGW(TAG, "encode called with null data for %s", this->get_name().c_str());
    return;
  }
  uint8_t value = *reinterpret_cast<uint8_t*>(data);
  encode(raw, length, value);
}

void OPTOLINKSelect::encode(uint8_t* raw, uint8_t length, uint8_t data) {
  if (_length == 0) {
    ESP_LOGW(TAG, "Unsupported zero length for %s", this->get_name().c_str());
    return;
  }
  if (length < _length) {
    ESP_LOGW(TAG, "encode length mismatch for %s: got=%u expected=%u",
             this->get_name().c_str(), (unsigned) length, (unsigned) _length);
    return;
  }
  uint8_t value = data;

  if (!this->is_allowed_value_(value)) {
    ESP_LOGW(TAG, "Refusing to encode invalid option value %u for %s", static_cast<unsigned>(value), this->get_name().c_str());
    return;
  }

  ESP_LOGD(TAG, "encode %s raw=%u", this->get_name().c_str(), static_cast<unsigned>(value));

  if (_length == 1) {
    raw[0] = value;
  } else {
    ESP_LOGW(TAG, "Unsupported length %d", _length);
    return;
  }
}

}  // namespace vitoconnect
}  // namespace esphome
