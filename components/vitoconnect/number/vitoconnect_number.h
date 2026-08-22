#pragma once

#include "esphome/components/number/number.h"
#include "../vitoconnect_datapoint.h"
#include "esphome/core/hal.h"

namespace esphome {
namespace vitoconnect {

class OPTOLINKNumber : public number::Number, public Datapoint {

  public:
    OPTOLINKNumber();
    ~OPTOLINKNumber();

    virtual void control(float value) override;

    void decode(uint8_t* data, uint8_t length, Datapoint* dp = nullptr) override;
    void encode(uint8_t* raw, uint8_t length, void* data) override;
    void encode(uint8_t* raw, uint8_t length, float data);
    void encode(uint8_t* raw, uint8_t length) override;

    void setDivRatio(float div) { this->_div_ratio = div; }
    void setSigned(bool is_signed) { this->_signed = is_signed; }
    uint32_t getLastReadMs() const { return this->_last_read_ms; }
    bool hasPendingCommand() const {
      return this->_has_command_value || this->_last_update != 0 ||
             this->_write_in_flight || this->_verify_pending;
    }
    bool hasActiveWriteCommand() const {
      return this->_write_in_flight || this->_verify_pending;
    }
    float getPendingCommandValue() const { return this->_command_value; }

  private:
    float _div_ratio = 1.0f;
    bool _signed = false;
    float _command_value = 0.0f;
    bool _has_command_value = false;
    uint32_t _last_read_ms = 0;
};

}  // namespace vitoconnect
}  // namespace esphome
