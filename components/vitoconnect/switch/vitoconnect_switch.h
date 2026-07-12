#pragma once

#include "esphome/components/switch/switch.h"
#include "../vitoconnect_datapoint.h"

namespace esphome {
namespace vitoconnect {

class VitoConnect;

class OPTOLINKSwitch : public switch_::Switch, public Datapoint {
 public:
  void set_parent(VitoConnect *parent) { this->parent_ = parent; }

  void decode(uint8_t *data, uint8_t length, Datapoint *dp = nullptr) override;
  void encode(uint8_t *raw, uint8_t length, void *data) override;

 protected:
  void write_state(bool state) override;

  VitoConnect *parent_{nullptr};
};

}  // namespace vitoconnect
}  // namespace esphome
