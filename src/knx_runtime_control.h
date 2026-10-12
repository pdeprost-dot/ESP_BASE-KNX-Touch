#pragma once

#include <stdint.h>

class KnxRuntimeControl {
 public:
  constexpr bool active() const { return active_; }
  constexpr uint32_t generation() const { return generation_; }

  constexpr void setActive(bool active) {
    if (active_ == active) return;
    active_ = active;
    ++generation_;
  }

  constexpr void forcePassive() { setActive(false); }

  constexpr bool mayTransmit(bool buttonEnabled, bool networkOperational,
                             bool pending) const {
    return active_ && buttonEnabled && networkOperational && !pending;
  }

 private:
  bool active_ = false;
  uint32_t generation_ = 0;
};
