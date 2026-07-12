#pragma once

#include "esphome/components/number/number.h"
#include "../vitoconnect_datapoint.h"

namespace esphome {
namespace vitoconnect {

class VitoConnect;

class OPTOLINKNumber : public number::Number, public Datapoint {
 public:
  void set_parent(VitoConnect *parent) { this->parent_ = parent; }

  void decode(uint8_t *data, uint8_t length, Datapoint *dp = nullptr) override;
  void encode(uint8_t *raw, uint8_t length, void *data) override;
  void encode(uint8_t *raw, uint8_t length, float data);

 protected:
  void control(float value) override;

  VitoConnect *parent_{nullptr};
};

}  // namespace vitoconnect
}  // namespace esphome
