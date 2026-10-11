#pragma once

#include <Arduino.h>

#include "button_model.h"
#include "lcd_driver.h"
#include "touch_driver.h"
#include "touch_release_gate.h"

class FourButtonUi {
 public:
  using ButtonActionHandler = void (*)(uint8_t index);

  bool begin(LcdDriver& lcd);
  void process(const TouchSample& sample);
  void setButtonActionHandler(ButtonActionHandler handler);
  void setButtonName(uint8_t index, const char* name);
  void setButtonState(uint8_t index, ButtonState state);

 private:
  int8_t hitTest(uint16_t x, uint16_t y) const;
  void renderAll();
  void renderButton(uint8_t index, bool pressed = false);
  const char* stateLabel(ButtonState state) const;
  uint16_t stateColor(ButtonState state) const;

  LcdDriver* lcd_ = nullptr;
  ButtonModel buttons_[4] = {
      {"Salon", ButtonState::Unknown},
      {"Cuisine", ButtonState::Unknown},
      {"Extérieur", ButtonState::Unknown},
      {"Garage", ButtonState::Unknown},
  };
  bool touchWasPressed_ = false;
  int8_t capturedButton_ = -1;
  uint32_t pressStartedMs_ = 0;
  bool longReported_ = false;
  ButtonActionHandler actionHandler_ = nullptr;
  TouchReleaseGate releaseGate_;
};

