#pragma once

#include "esphome/components/select/select.h"
#include "../vitoconnect_datapoint.h"
#include "esphome/core/hal.h"
#include <vector>
#include <string>

namespace esphome {
namespace vitoconnect {

class OPTOLINKSelect : public select::Select, public Datapoint {

 public:
  OPTOLINKSelect();
  ~OPTOLINKSelect();

  void control(const std::string &value) override;

  void decode(uint8_t* data, uint8_t length, Datapoint* dp = nullptr) override;
  void encode(uint8_t* raw, uint8_t length) override;
  void encode(uint8_t* raw, uint8_t length, void* data) override;
  void encode(uint8_t* raw, uint8_t length, uint8_t data);

  // setters from Python config
  void set_option_labels(const std::vector<std::string> &labels);
  void set_option_values(const std::vector<uint8_t> &values);
 private:
  std::vector<std::string> option_labels_;
  std::vector<uint8_t> option_values_;
  uint8_t current_value_ = 0;
};

}  // namespace vitoconnect
}  // namespace esphome