#pragma once

#include "esphome/components/text_sensor/text_sensor.h"
#include "../vitoconnect_datapoint.h"
#include <map>
#include <string>

namespace esphome {
namespace vitoconnect {

class OPTOLINKTextSensor : public text_sensor::TextSensor, public Datapoint {

 public:
  OPTOLINKTextSensor();
  ~OPTOLINKTextSensor();

  void add_mapping(uint16_t value, const std::string &label) { this->value_map_[value] = label; }
  void set_unknown_value(const std::string &unknown_value) { this->unknown_value_ = unknown_value; }

  void decode(uint8_t *data, uint8_t length, Datapoint *dp = nullptr) override;
  void encode(uint8_t *raw, uint8_t length, void *data) override;

 protected:
  std::map<uint16_t, std::string> value_map_;
  std::string unknown_value_;
};

}  // namespace vitoconnect
}  // namespace esphome
