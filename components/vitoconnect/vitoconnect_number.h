#pragma once

#include "esphome/components/number/number.h"
#include "esphome/core/component.h"
#include "vitoconnect_datapoint.h"

namespace esphome {
namespace vitoconnect {

class VitoConnect;

class VitoNumber : public number::Number, public Component {
public:
  void setup() override;
  void dump_config() override;
  void control(float value) override;

  void set_address(uint16_t address) { this->address_ = address; }
  void set_length(uint8_t length) { this->length_ = length; }
  void set_parent(VitoConnect *parent) { this->parent_ = parent; }
  uint16_t get_address() const { return this->address_; }
  uint8_t get_length() const { return this->length_; }

  // This will be called by VitoConnect when data is received
  void on_update(const std::vector<uint8_t> &data);

protected:
  uint16_t address_;
  uint8_t length_;
  VitoConnect *parent_;
};

} // namespace vitoconnect
} // namespace esphome
