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

  uint32_t getLastReadMs() const { return this->_last_read_ms; }
  bool hasPendingCommand() const {
    return this->_has_command_value || this->_last_update != 0 ||
           this->_write_in_flight || this->_verify_pending;
  }
  bool hasActiveWriteCommand() const {
    return this->_write_in_flight || this->_verify_pending;
  }
  uint8_t getPendingCommandValue() const { return this->_command_value; }

 private:
  int find_label_index_(const std::string &label) const;
  int find_value_index_(uint8_t value) const;
  std::string label_for_value_(uint8_t value) const;
  bool is_allowed_value_(uint8_t value) const;
  void stage_value_(uint8_t value, const std::string &publish_value);

  std::vector<std::string> option_labels_;
  std::vector<uint8_t> option_values_;
  uint8_t current_value_ = 0;
  uint8_t _command_value = 0;
  bool _has_command_value = false;
  uint32_t _last_read_ms = 0;
};

}  // namespace vitoconnect
}  // namespace esphome
