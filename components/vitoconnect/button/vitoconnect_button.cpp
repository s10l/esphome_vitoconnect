#include "vitoconnect_button.h"
#include "../vitoconnect.h"
#include "esphome/core/log.h"

namespace esphome {
namespace vitoconnect {

static const char *TAG = "vitoconnect_button";

void OPTOLINKRefreshOnceButton::press_action() {
  if (this->parent_ == nullptr) {
    ESP_LOGW(TAG, "No VitoConnect parent set");
    return;
  }
  this->parent_->refresh_once_datapoints();
}

}  // namespace vitoconnect
}  // namespace esphome
