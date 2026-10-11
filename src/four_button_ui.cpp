#include "four_button_ui.h"

namespace {
constexpr uint16_t kBlack = 0x0000;
constexpr uint16_t kWhite = 0xFFFF;
constexpr uint16_t kUnknown = 0x7BEF;
constexpr uint16_t kOff = 0x0014;
constexpr uint16_t kOn = 0x0600;
constexpr uint16_t kPending = 0xFD20;
constexpr int16_t kMargin = 4;
constexpr int16_t kGap = 4;
constexpr int16_t kButtonWidth = 154;
constexpr int16_t kButtonHeight = 80;
constexpr uint32_t kLongPressMs = 800;
}  // namespace

bool FourButtonUi::begin(LcdDriver& lcd) {
  lcd_ = &lcd;
  renderAll();
  Serial.println("[UI][READY] four independent KNX models; transmit=BLOCKED");
  return true;
}

void FourButtonUi::setButtonName(uint8_t index, const char* name) {
  if (index >= 4 || !name) return;
  buttons_[index].name = name;
  if (lcd_) renderButton(index);
}

void FourButtonUi::setButtonActionHandler(ButtonActionHandler handler) {
  actionHandler_ = handler;
}

void FourButtonUi::setButtonState(uint8_t index, ButtonState state) {
  if (index >= 4 || buttons_[index].state == state) return;
  buttons_[index].state = state;
  if (lcd_) renderButton(index);
}

int8_t FourButtonUi::hitTest(uint16_t x, uint16_t y) const {
  for (uint8_t index = 0; index < 4; ++index) {
    const int16_t left = kMargin + (index % 2) * (kButtonWidth + kGap);
    const int16_t top = kMargin + (index / 2) * (kButtonHeight + kGap);
    if (x >= left && x < left + kButtonWidth && y >= top && y < top + kButtonHeight) {
      return index;
    }
  }
  return -1;
}

const char* FourButtonUi::stateLabel(ButtonState state) const {
  switch (state) {
    case ButtonState::On: return "ON";
    case ButtonState::Off: return "OFF";
    case ButtonState::Pending: return "APPUI";
    case ButtonState::Unknown: return "INCONNU";
  }
  return "?";
}

uint16_t FourButtonUi::stateColor(ButtonState state) const {
  switch (state) {
    case ButtonState::On: return kOn;
    case ButtonState::Off: return kOff;
    case ButtonState::Pending: return kPending;
    case ButtonState::Unknown: return kUnknown;
  }
  return kUnknown;
}

void FourButtonUi::renderButton(uint8_t index, bool pressed) {
  const int16_t left = kMargin + (index % 2) * (kButtonWidth + kGap);
  const int16_t top = kMargin + (index / 2) * (kButtonHeight + kGap);
  const ButtonState renderedState = pressed ? ButtonState::Pending : buttons_[index].state;
  lcd_->fillRect(left, top, kButtonWidth, kButtonHeight, stateColor(renderedState));
  lcd_->drawRect(left, top, kButtonWidth, kButtonHeight, pressed ? kBlack : kWhite);
  lcd_->centeredText(buttons_[index].name, left + kButtonWidth / 2, top + 18, kWhite, 2);
  lcd_->centeredText(stateLabel(renderedState), left + kButtonWidth / 2, top + 50,
                     renderedState == ButtonState::Pending ? kBlack : kWhite, 2);
}

void FourButtonUi::renderAll() {
  lcd_->fill(kBlack);
  for (uint8_t index = 0; index < 4; ++index) {
    renderButton(index);
  }
}

void FourButtonUi::process(const TouchSample& sample) {
  if (sample.pressed && !touchWasPressed_) {
    capturedButton_ = hitTest(sample.x, sample.y);
    releaseGate_.capture(capturedButton_);
    pressStartedMs_ = sample.timestampMs;
    longReported_ = false;
    if (capturedButton_ >= 0) {
      renderButton(capturedButton_, true);
      Serial.printf("[UI][PRESS] button=%d name=%s x=%u y=%u\n", capturedButton_,
                    buttons_[capturedButton_].name, sample.x, sample.y);
    } else {
      Serial.printf("[UI][PRESS_OUTSIDE] x=%u y=%u\n", sample.x, sample.y);
    }
  } else if (sample.pressed && touchWasPressed_ && capturedButton_ >= 0 &&
             !longReported_ && sample.timestampMs - pressStartedMs_ >= kLongPressMs) {
    longReported_ = true;
    Serial.printf("[UI][LONG] button=%d name=%s duration_ms=%lu\n", capturedButton_,
                  buttons_[capturedButton_].name,
                  static_cast<unsigned long>(sample.timestampMs - pressStartedMs_));
  } else if (!sample.pressed && touchWasPressed_) {
    if (capturedButton_ >= 0) {
      const int8_t releasedButton = hitTest(sample.x, sample.y);
      const int8_t actionButton = releaseGate_.complete(releasedButton);
      if (actionButton == capturedButton_) {
        Serial.printf("[UI][ACTION] button=%d name=%s duration_ms=%lu\n",
                      capturedButton_, buttons_[capturedButton_].name,
                      static_cast<unsigned long>(sample.timestampMs - pressStartedMs_));
        if (actionHandler_) actionHandler_(capturedButton_);
      } else {
        Serial.printf("[UI][CANCEL] captured=%d release_zone=%d\n",
                      capturedButton_, releasedButton);
      }
      renderButton(capturedButton_);
    }
    capturedButton_ = -1;
  }
  touchWasPressed_ = sample.pressed;
}

