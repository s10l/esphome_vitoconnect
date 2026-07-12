#include "vitoconnect_switch.h"
#include "../vitoconnect.h"
#include "esphome/core/log.h"

namespace esphome {
namespace vitoconnect {

static const char *TAG = "vitoconnect_switch";

void OPTOLINKSwitch::write_state(bool state) {
  if (this->parent_ == nullptr) {
    ESP_LOGW(TAG, "No VitoConnect parent set for datapoint 0x%04X", this->getAddress());
    return;
  }
  if (!this->parent_->write_datapoint(this, &state)) {
    ESP_LOGW(TAG, "Failed to queue write for datapoint 0x%04X", this->getAddress());
  }
}

void OPTOLINKSwitch::decode(uint8_t *data, uint8_t length, Datapoint *dp) {
  assert(length >= this->_length);
  this->publish_state(data[0] != 0);
}

void OPTOLINKSwitch::encode(uint8_t *raw, uint8_t length, void *data) {
  bool value = *reinterpret_cast<bool *>(data);
  memset(raw, 0, length);
  raw[0] = value ? 1 : 0;
}

}  // namespace vitoconnect
}  // namespace esphome
