#include "vitoconnect_number.h"
#include "vitoconnect.h"

namespace esphome {
namespace vitoconnect {

static const char *TAG = "vitoconnect.number";

void VitoNumber::setup() {
  // Optional: Restore state etc.
}

void VitoNumber::dump_config() {
  LOG_NUMBER("", "VitoNumber", this);
  ESP_LOGCONFIG(TAG, "  Address: 0x%04X", this->address_);
  ESP_LOGCONFIG(TAG, "  Length: %d", this->length_);
}

void VitoNumber::control(float value) {
  // Publish state to frontend immediately for better UX
  this->publish_state(value);

  int32_t val_int = static_cast<int32_t>(value);
  std::vector<uint8_t> payload;

  if (this->length_ == 1) {
    payload.push_back(static_cast<uint8_t>(val_int & 0xFF));
  } else if (this->length_ == 2) {
    // Little Endian usually for Viessmann P300?
    // Reference projects often swap if needed.
    // Let's assume standard LE for now.
    payload.push_back(static_cast<uint8_t>(val_int & 0xFF));
    payload.push_back(static_cast<uint8_t>((val_int >> 8) & 0xFF));
  }

  if (this->parent_ != nullptr) {
    this->parent_->write_datapoint(this->address_, this->length_, payload);
  }
}

// Logic needs Parent pointer to call write_datapoint!
// Updating .h to include parent.

void VitoNumber::on_update(const std::vector<uint8_t> &data) {
  if (data.empty())
    return;

  float new_value = 0;
  if (data.size() == 1) {
    new_value = static_cast<float>(data[0]);
  } else if (data.size() >= 2) {
    // Little Endian
    int16_t val = data[0] | (data[1] << 8);
    new_value = static_cast<float>(val);
  }

  this->publish_state(new_value);
}

} // namespace vitoconnect
} // namespace esphome
