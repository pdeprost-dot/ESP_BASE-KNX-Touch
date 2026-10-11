#include "touch_driver.h"

#include <Wire.h>

namespace {
constexpr uint8_t kAddress = 0x63;
constexpr uint8_t kIdRegister = 0x08;
constexpr uint8_t kDataRegister = 0x01;
constexpr int kSdaPin = 42;
constexpr int kSclPin = 41;
constexpr int kResetPin = 47;
constexpr int kInterruptPin = 48;
constexpr uint32_t kPollIntervalMs = 20;
}  // namespace

bool TouchDriver::readRegister(uint8_t reg, uint8_t* data, size_t length) {
  Wire.beginTransmission(kAddress);
  Wire.write(reg);
  const uint8_t status = Wire.endTransmission();
  if (status != 0) {
    ++errors_;
    Serial.printf("[TOUCH][I2C_ERROR] reg=0x%02X status=%u\n", reg, status);
    return false;
  }

  const size_t received = Wire.requestFrom(kAddress, length);
  if (received != length) {
    ++errors_;
    Serial.printf("[TOUCH][I2C_ERROR] reg=0x%02X expected=%u received=%u\n",
                  reg, static_cast<unsigned>(length), static_cast<unsigned>(received));
    while (Wire.available()) {
      Wire.read();
    }
    return false;
  }
  for (size_t i = 0; i < length; ++i) {
    data[i] = Wire.read();
  }
  ++successfulReads_;
  return true;
}

bool TouchDriver::begin() {
  pinMode(kInterruptPin, INPUT_PULLUP);
  pinMode(kResetPin, OUTPUT);
  digitalWrite(kResetPin, LOW);
  delay(200);
  digitalWrite(kResetPin, HIGH);
  delay(300);

  if (!Wire.begin(kSdaPin, kSclPin, 400000)) {
    Serial.println("[TOUCH][FAIL] I2C initialization");
    return false;
  }
  Wire.setTimeOut(50);

  Wire.beginTransmission(kAddress);
  const uint8_t probeStatus = Wire.endTransmission();
  if (probeStatus != 0) {
    Serial.printf("[TOUCH][FAIL] address=0x%02X status=%u\n", kAddress, probeStatus);
    return false;
  }

  uint8_t id[3] = {0};
  if (!readRegister(kIdRegister, id, sizeof(id))) {
    Serial.println("[TOUCH][FAIL] ID read");
    return false;
  }
  Serial.printf("[TOUCH][READY] SDA=%d SCL=%d RST=%d INT=%d address=0x%02X id=%02X%02X%02X\n",
                kSdaPin, kSclPin, kResetPin, kInterruptPin, kAddress,
                id[0], id[1], id[2]);
  ready_ = true;
  return true;
}

bool TouchDriver::poll(TouchSample& sample) {
  const uint32_t now = millis();
  if (!ready_ || now - lastPollMs_ < kPollIntervalMs) {
    return false;
  }
  lastPollMs_ = now;

  uint8_t data[14] = {0};
  if (!readRegister(kDataRegister, data, sizeof(data))) {
    return false;
  }

  const bool pressed = data[1] > 0;
  if (pressed) {
    const uint16_t rawX = static_cast<uint16_t>(data[2] & 0x0F) << 8 | data[3];
    const uint16_t rawY = static_cast<uint16_t>(data[4] & 0x0F) << 8 | data[5];
    // Landscape rotation 1, identical to the validated Waveshare transform.
    lastX_ = rawY;
    lastY_ = rawX;
  }

  sample = {pressed, lastX_, lastY_, now};
  lastPressed_ = pressed;
  return true;
}

uint32_t TouchDriver::successfulReads() const {
  return successfulReads_;
}

uint32_t TouchDriver::errors() const {
  return errors_;
}

