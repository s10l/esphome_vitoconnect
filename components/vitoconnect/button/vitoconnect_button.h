#pragma once

#include "esphome/components/button/button.h"

namespace esphome {
namespace vitoconnect {

class VitoConnect;

class OPTOLINKRefreshOnceButton : public button::Button {
 public:
  void set_parent(VitoConnect *parent) { this->parent_ = parent; }

 protected:
  void press_action() override;

  VitoConnect *parent_{nullptr};
};

}  // namespace vitoconnect
}  // namespace esphome
