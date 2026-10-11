#pragma once

#include <Arduino.h>

enum class ButtonState : uint8_t {
  Unknown,
  Off,
  On,
  Pending,
};

struct ButtonModel {
  const char* name;
  ButtonState state;
};

inline ButtonState toggledLocalState(ButtonState state) {
  return state == ButtonState::On ? ButtonState::Off : ButtonState::On;
}

