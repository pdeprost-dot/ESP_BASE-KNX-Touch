#pragma once

#include <Arduino.h>

struct TouchSample {
  bool pressed;
  uint16_t x;
  uint16_t y;
  uint32_t timestampMs;
};

class TouchDriver {
 public:
  bool begin();
  bool poll(TouchSample& sample);
  uint32_t successfulReads() const;
  uint32_t errors() const;

 private:
  bool readRegister(uint8_t reg, uint8_t* data, size_t length);

  bool ready_ = false;
  bool lastPressed_ = false;
  uint16_t lastX_ = 0;
  uint16_t lastY_ = 0;
  uint32_t lastPollMs_ = 0;
  uint32_t successfulReads_ = 0;
  uint32_t errors_ = 0;
};

