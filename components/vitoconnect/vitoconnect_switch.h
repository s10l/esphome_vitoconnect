#pragma once

#include "esphome/components/switch/switch.h"
#include "esphome/core/component.h"
// #include "vitoconnect.h" - Removed to avoid circular dependency

namespace esphome {
namespace vitoconnect {

class VitoConnect;

class VitoSwitch : public switch_::Switch, public Component {
public:
  void set_parent(VitoConnect *parent) { this->parent_ = parent; }
  void set_address(uint16_t address) { this->address_ = address; }
  void set_length(uint8_t length) { this->length_ = length; }
  void set_on_value(std::vector<uint8_t> on_value) {
    this->on_value_ = on_value;
  }
  void set_off_value(std::vector<uint8_t> off_value) {
    this->off_value_ = off_value;
  }

  void setup() override;
  void dump_config() override;

  // Called when user toggles switch in HA
  void write_state(bool state) override;

protected:
  VitoConnect *parent_;
  uint16_t address_;
  uint8_t length_;
  std::vector<uint8_t> on_value_;
  std::vector<uint8_t> off_value_;
};

} // namespace vitoconnect
} // namespace esphome
