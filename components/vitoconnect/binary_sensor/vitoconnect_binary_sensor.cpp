#include "vitoconnect_binary_sensor.h"

namespace esphome {
namespace vitoconnect {

static const char *TAG = "vitoconnect.binary_sensor";

OPTOLINKBinarySensor::OPTOLINKBinarySensor(){
  // default bit mask already set in header initializer
}

OPTOLINKBinarySensor::~OPTOLINKBinarySensor() {
  // empty
}

void OPTOLINKBinarySensor::decode(uint8_t* data, uint8_t length, Datapoint* dp) {
  if (length < _length) {
    ESP_LOGW(TAG, "decode length mismatch for %s: got=%u expected=%u",
             this->get_name().c_str(), (unsigned) length, (unsigned) _length);
    return;
  }

  if (!dp) dp = this;

  // Apply bit mask: publish true if any of the masked bits are set
  bool active = (data[0] & _bit_mask_) != 0;
  publish_state(active);
}

void OPTOLINKBinarySensor::encode(uint8_t* raw, uint8_t length, void* data) {
  float value = *reinterpret_cast<float*>(data);
  encode(raw, length, value);
}

void OPTOLINKBinarySensor::encode(uint8_t* raw, uint8_t length, float data) {
  if (length < _length) {
    ESP_LOGW(TAG, "encode length mismatch for %s: got=%u expected=%u",
             this->get_name().c_str(), (unsigned) length, (unsigned) _length);
    return;
  }

}

void OPTOLINKBinarySensor::setBitMask(uint8_t mask) {
  this->_bit_mask_ = mask;
}

}  // namespace vitoconnect
}  // namespace esphome
