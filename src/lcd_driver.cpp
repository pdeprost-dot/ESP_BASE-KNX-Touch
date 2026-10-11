#include "lcd_driver.h"

#include <Arduino_GFX_Library.h>

namespace {
constexpr int kBacklightPin = 46;
Arduino_DataBus* bus = new Arduino_ESP32SPI(45, 21, 38, 39);
Arduino_GFX* gfx = new Arduino_ST7789(bus, 40, 0, false, 172, 320, 34, 0, 34, 0);

void initializeJd9853() {
  static const uint8_t operations[] = {
      BEGIN_WRITE, WRITE_COMMAND_8, 0x11, END_WRITE, DELAY, 120,
      BEGIN_WRITE,
      WRITE_C8_D16, 0xDF, 0x98, 0x53,
      WRITE_C8_D8, 0xB2, 0x23,
      WRITE_COMMAND_8, 0xB7, WRITE_BYTES, 4, 0x00, 0x47, 0x00, 0x6F,
      WRITE_COMMAND_8, 0xBB, WRITE_BYTES, 6, 0x1C, 0x1A, 0x55, 0x73, 0x63, 0xF0,
      WRITE_C8_D16, 0xC0, 0x44, 0xA4,
      WRITE_C8_D8, 0xC1, 0x16,
      WRITE_COMMAND_8, 0xC3, WRITE_BYTES, 8,
      0x7D, 0x07, 0x14, 0x06, 0xCF, 0x71, 0x72, 0x77,
      WRITE_COMMAND_8, 0xC4, WRITE_BYTES, 12,
      0x00, 0x00, 0xA0, 0x79, 0x0B, 0x0A, 0x16, 0x79, 0x0B, 0x0A, 0x16, 0x82,
      WRITE_COMMAND_8, 0xC8, WRITE_BYTES, 32,
      0x3F, 0x32, 0x29, 0x29, 0x27, 0x2B, 0x27, 0x28,
      0x28, 0x26, 0x25, 0x17, 0x12, 0x0D, 0x04, 0x00,
      0x3F, 0x32, 0x29, 0x29, 0x27, 0x2B, 0x27, 0x28,
      0x28, 0x26, 0x25, 0x17, 0x12, 0x0D, 0x04, 0x00,
      WRITE_COMMAND_8, 0xD0, WRITE_BYTES, 5, 0x04, 0x06, 0x6B, 0x0F, 0x00,
      WRITE_C8_D16, 0xD7, 0x00, 0x30,
      WRITE_C8_D8, 0xE6, 0x14,
      WRITE_C8_D8, 0xDE, 0x01,
      WRITE_COMMAND_8, 0xB7, WRITE_BYTES, 5, 0x03, 0x13, 0xEF, 0x35, 0x35,
      WRITE_COMMAND_8, 0xC1, WRITE_BYTES, 3, 0x14, 0x15, 0xC0,
      WRITE_C8_D16, 0xC2, 0x06, 0x3A,
      WRITE_C8_D16, 0xC4, 0x72, 0x12,
      WRITE_C8_D8, 0xBE, 0x00,
      WRITE_C8_D8, 0xDE, 0x02,
      WRITE_COMMAND_8, 0xE5, WRITE_BYTES, 3, 0x00, 0x02, 0x00,
      WRITE_COMMAND_8, 0xE5, WRITE_BYTES, 3, 0x01, 0x02, 0x00,
      WRITE_C8_D8, 0xDE, 0x00,
      WRITE_C8_D8, 0x35, 0x00,
      WRITE_C8_D8, 0x3A, 0x05,
      WRITE_COMMAND_8, 0x2A, WRITE_BYTES, 4, 0x00, 0x22, 0x00, 0xCD,
      WRITE_COMMAND_8, 0x2B, WRITE_BYTES, 4, 0x00, 0x00, 0x01, 0x3F,
      WRITE_C8_D8, 0xDE, 0x02,
      WRITE_COMMAND_8, 0xE5, WRITE_BYTES, 3, 0x00, 0x02, 0x00,
      WRITE_C8_D8, 0xDE, 0x00,
      WRITE_C8_D8, 0x36, 0x00,
      WRITE_COMMAND_8, 0x21,
      END_WRITE, DELAY, 10,
      BEGIN_WRITE, WRITE_COMMAND_8, 0x29, END_WRITE,
  };
  bus->batchOperation(operations, sizeof(operations));
}
}  // namespace

bool LcdDriver::begin() {
  if (!gfx->begin()) {
    Serial.println("[LCD][FAIL] gfx begin");
    return false;
  }
  initializeJd9853();
  gfx->setRotation(1);
  gfx->fillScreen(BLACK);
  pinMode(kBacklightPin, OUTPUT);
  digitalWrite(kBacklightPin, HIGH);
  Serial.printf("[LCD][READY] width=%d height=%d rotation=1\n", gfx->width(), gfx->height());
  return true;
}

void LcdDriver::fill(uint16_t color) { gfx->fillScreen(color); }
void LcdDriver::fillRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color) {
  gfx->fillRect(x, y, w, h, color);
}
void LcdDriver::drawRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color) {
  gfx->drawRect(x, y, w, h, color);
}
void LcdDriver::centeredText(const char* text, int16_t centerX, int16_t baselineY,
                             uint16_t color, uint8_t size) {
  gfx->setTextSize(size);
  gfx->setTextColor(color);
  gfx->setCursor(centerX - static_cast<int16_t>(strlen(text) * 3 * size), baselineY);
  gfx->print(text);
}
uint16_t LcdDriver::width() const { return gfx->width(); }
uint16_t LcdDriver::height() const { return gfx->height(); }

