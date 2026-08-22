#include "vitoconnect_number.h"

#include <cmath>
#include <cstdint>
#include <limits>


namespace esphome {
namespace vitoconnect {

static const char *TAG = "vitoconnect.number";

OPTOLINKNumber::OPTOLINKNumber(){}

OPTOLINKNumber::~OPTOLINKNumber() {}

void OPTOLINKNumber::control(float value) {
  const float min_value = this->traits.get_min_value();
  const float max_value = this->traits.get_max_value();

  if (value < min_value) {
    ESP_LOGW(TAG, "control value of number %s below min_value", this->get_name().c_str());
    value = min_value;
  }
  if (value > max_value) {
    ESP_LOGW(TAG, "control value of number %s above max_value", this->get_name().c_str());
    value = max_value;
  }

  float step = this->traits.get_step();
  if (step > 0.0f) {
    float tmp = min_value + std::round((value - min_value) / step) * step;
    if (tmp != value) {
      ESP_LOGW(TAG, "control value of number %s not matching step %f", this->get_name().c_str(), step);
      value = tmp;
    }
  }

  if (value < min_value) {
    ESP_LOGW(TAG, "control value of number %s below min_value after step quantization", this->get_name().c_str());
    value = min_value;
  }
  if (value > max_value) {
    ESP_LOGW(TAG, "control value of number %s above max_value after step quantization", this->get_name().c_str());
    value = max_value;
  }

  ESP_LOGD(TAG, "state of number %s to value: %f", this->get_name().c_str(), value);

  this->_command_value = value;
  this->_has_command_value = true;
  this->_last_update = millis();
}

void OPTOLINKNumber::decode(uint8_t* data, uint8_t length, Datapoint* dp) {
  if (_length == 0 || _length > 8) {
    ESP_LOGE(TAG, "Unsupported number length %u for %s (must be 1..8)",
             (unsigned) _length, this->get_name().c_str());
    return;
  }
  if (length < _length) {
    ESP_LOGW(TAG, "decode length mismatch for %s: got=%u expected=%u",
             this->get_name().c_str(), (unsigned) length, (unsigned) _length);
    return;
  }

  if (this->_div_ratio <= 0.0f) {
    ESP_LOGW(TAG, "Invalid div_ratio (%f) for number %s; forcing to 1.0", (double) this->_div_ratio, this->get_name().c_str());
    this->_div_ratio = 1.0f;
  }

  // Build little-endian unsigned integer
  uint64_t u = 0;
  for (uint8_t i = 0; i < _length; i++) {
    u |= (uint64_t) data[i] << (8 * i);
  }

  // Interpret as signed or unsigned
  int64_t iv = 0;
  if (this->_signed) {
    const uint8_t bits = _length * 8;
    const uint64_t sign_bit = 1ULL << (bits - 1);
    const uint64_t mask = (bits == 64) ? 0xFFFFFFFFFFFFFFFFULL : ((1ULL << bits) - 1ULL);

    if (u & sign_bit) {
      // sign-extend
      iv = (int64_t) (u | (~mask));
    } else {
      iv = (int64_t) u;
    }
  } else {
    const uint64_t max_i64 = static_cast<uint64_t>(std::numeric_limits<int64_t>::max());
    if (_length == 8 && u > max_i64) {
      ESP_LOGW(TAG, "decode %s: unsigned 64-bit raw=%llu above INT64_MAX; clamping",
               this->get_name().c_str(), (unsigned long long) u);
      u = max_i64;
    }
    iv = (int64_t) u;
  }

  float value = ((float) iv) / this->_div_ratio;
  ESP_LOGD(TAG, "decode %s raw=%lld div_ratio=%f -> %f", this->get_name().c_str(), (long long) iv, (double) this->_div_ratio, value);
  if (this->_has_command_value && fabsf(value - this->_command_value) < 0.0001f) {
    this->_has_command_value = false;
  }
  this->_last_read_ms = millis();
  publish_state(value);
}

void OPTOLINKNumber::encode(uint8_t* raw, uint8_t length) {
  float value = this->_has_command_value ? this->_command_value : this->state;
  encode(raw, length, value);
}

void OPTOLINKNumber::encode(uint8_t* raw, uint8_t length, void* data) {
  float value = *reinterpret_cast<float*>(data);
  encode(raw, length, value);
}

void OPTOLINKNumber::encode(uint8_t* raw, uint8_t length, float data) {
  if (_length == 0 || _length > 8) {
    ESP_LOGE(TAG, "Unsupported number length %u for %s (must be 1..8)",
             (unsigned) _length, this->get_name().c_str());
    return;
  }
  if (length < _length) {
    ESP_LOGW(TAG, "encode length mismatch for %s: got=%u expected=%u",
             this->get_name().c_str(), (unsigned) length, (unsigned) _length);
    return;
  }

  if (this->_div_ratio <= 0.0f) {
    ESP_LOGW(TAG, "Invalid div_ratio (%f) for number %s; forcing to 1.0", (double) this->_div_ratio, this->get_name().c_str());
    this->_div_ratio = 1.0f;
  }

  const uint8_t bits = _length * 8;

  // Scale from displayed units to raw units
  const double scaled = (double) data * (double) this->_div_ratio;

  // Round to nearest integer (correct for negatives as well)
  int64_t iv = (int64_t) llround(scaled);

  // Clamp to representable range
  int64_t min_v = 0;
  int64_t max_v = 0;
  uint64_t mask = 0;

  if (this->_signed) {
    if (bits == 64) {
      min_v = std::numeric_limits<int64_t>::min();
      max_v = std::numeric_limits<int64_t>::max();
      mask = 0xFFFFFFFFFFFFFFFFULL;
    } else {
      min_v = -(1LL << (bits - 1));
      max_v =  (1LL << (bits - 1)) - 1;
      mask = (1ULL << bits) - 1ULL;
    }
  } else {
    min_v = 0;
    const uint64_t max_u = (bits == 64) ? 0xFFFFFFFFFFFFFFFFULL : ((1ULL << bits) - 1ULL);
    max_v = (bits == 64) ? std::numeric_limits<int64_t>::max() : (int64_t) max_u;
    mask = max_u;
  }

  if (iv < min_v) {
    ESP_LOGW(TAG, "encode %s: value %f (scaled=%f) below representable min %lld; clamping", this->get_name().c_str(), data, scaled, (long long) min_v);
    iv = min_v;
  } else if (iv > max_v) {
    ESP_LOGW(TAG, "encode %s: value %f (scaled=%f) above representable max %lld; clamping", this->get_name().c_str(), data, scaled, (long long) max_v);
    iv = max_v;
  }

  // Convert to little-endian bytes
  uint64_t u = 0;
  if (this->_signed) {
    u = ((uint64_t) iv) & mask;
  } else {
    u = (uint64_t) iv;
  }

  for (uint8_t i = 0; i < _length; i++) {
    raw[i] = (uint8_t) ((u >> (8 * i)) & 0xFF);
  }

  ESP_LOGD(TAG, "encode %s value=%f div_ratio=%f -> raw=%lld", this->get_name().c_str(), data, (double) this->_div_ratio, (long long) iv);
}

}  // namespace vitoconnect
}  // namespace esphome
