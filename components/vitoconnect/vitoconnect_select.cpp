#include "vitoconnect_select.h"
#include "esphome/core/log.h"
#include "vitoconnect.h"

namespace esphome {
namespace vitoconnect {

static const char *TAG = "vitoconnect.select";

void VitoSelect::setup() {
  // Restore if needed
}

void VitoSelect::dump_config() {
  LOG_SELECT("", "VitoSelect", this);
  ESP_LOGCONFIG(TAG, "  Address: 0x%04X", this->address_);
  ESP_LOGCONFIG(TAG, "  Length: %d", this->length_);
}

void VitoSelect::control(const std::string &value) {
  this->publish_state(value);

  // Find index of value in options
  const auto &options = this->traits.get_options();
  auto it = std::find(options.begin(), options.end(), value);

  if (it != options.end()) {
    uint8_t index = std::distance(options.begin(), it);
    ESP_LOGD(TAG, "Select option '%s' mapped to index %d", value.c_str(),
             index);

    std::vector<uint8_t> payload;
    payload.push_back(index);

    if (this->parent_ != nullptr) {
      this->parent_->write_datapoint(this->address_, this->length_, payload);
    }
  } else {
    ESP_LOGW(TAG, "Invalid option selected: %s", value.c_str());
  }
}

void VitoSelect::on_update(const std::vector<uint8_t> &data) {
  if (data.empty())
    return;

  // Assume the first byte is the index
  uint8_t index = data[0];
  const auto &options = this->traits.get_options();

  if (index < options.size()) {
    this->publish_state(options[index]);
  } else {
    ESP_LOGW(TAG,
             "Received index %d out of bounds for select options (size %d)",
             index, options.size());
  }
}

} // namespace vitoconnect
} // namespace esphome
