#pragma once

#include <stdint.h>

class TouchReleaseGate {
 public:
  constexpr void capture(int8_t button) {
    captured_ = button;
    active_ = button >= 0;
  }

  // Returns one action only when the gesture ends in its captured zone.
  constexpr int8_t complete(int8_t releasedButton) {
    if (!active_) return -1;
    const int8_t action = releasedButton == captured_ ? captured_ : -1;
    active_ = false;
    captured_ = -1;
    return action;
  }

 private:
  int8_t captured_ = -1;
  bool active_ = false;
};

