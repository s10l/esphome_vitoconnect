#include "vitoconnect_switch.h"
#include "esphome/core/log.h"
#include "vitoconnect.h"

namespace esphome {
namespace vitoconnect {

static const char *TAG = "vitoconnect.switch";

void VitoSwitch::setup() {
  // Initialization if needed
}

void VitoSwitch::dump_config() {
  ESP_LOGCONFIG(TAG, "VitoSwitch:");
  ESP_LOGCONFIG(TAG, "  Address: 0x%04X", this->address_);
  ESP_LOGCONFIG(TAG, "  Length: %d", this->length_);
}

void VitoSwitch::write_state(bool state) {
  // 1. Update the frontend state immediately (optimistic)
  this->publish_state(state);

  // 2. Prepare data payload
  std::vector<uint8_t> payload = state ? this->on_value_ : this->off_value_;

  // 3. Ensure payload length matches defined length
  if (payload.size() != this->length_) {
    ESP_LOGE(TAG, "Payload size (%d) does not match switch length (%d)",
             payload.size(), this->length_);
    return;
  }

  // 4. Send to parent to write
  // We need to cast vector data to uint8_t array
  this->parent_->write_datapoint(this->address_, this->length_, payload.data());
}

} // namespace vitoconnect
} // namespace esphome
