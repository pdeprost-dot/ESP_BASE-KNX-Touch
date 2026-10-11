#pragma once

#include <Arduino.h>

class LcdDriver {
 public:
  bool begin();
  void fill(uint16_t color);
  void fillRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color);
  void drawRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color);
  void centeredText(const char* text, int16_t centerX, int16_t baselineY,
                    uint16_t color, uint8_t size);
  uint16_t width() const;
  uint16_t height() const;
};

