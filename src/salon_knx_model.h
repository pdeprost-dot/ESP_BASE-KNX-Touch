#pragma once

#include <stdint.h>

enum class SalonKnxState : uint8_t {
  Unknown,
  Off,
  On,
  Pending,
};

class SalonKnxModel {
 public:
  static constexpr uint32_t kDefaultFeedbackTimeoutMs = 5000;

  constexpr SalonKnxState state() const { return state_; }
  constexpr bool hasConfirmedState() const { return confirmedKnown_; }
  constexpr bool confirmedValue() const { return confirmedValue_; }
  constexpr bool hasLastConfirmedValue() const { return lastConfirmedKnown_; }
  constexpr bool lastConfirmedValue() const { return lastConfirmedValue_; }

  constexpr bool prepareCommand(bool& requestedValue) const {
    if (state_ == SalonKnxState::Pending) return false;
    requestedValue = state_ != SalonKnxState::On;
    return true;
  }

  constexpr void invalidateSynchronization() {
    confirmedKnown_ = false;
    state_ = SalonKnxState::Unknown;
  }

  // Called only after the transport has accepted an authorized command.
  constexpr void commandAccepted(bool requestedValue, uint32_t nowMs) {
    requestedValue_ = requestedValue;
    pendingSinceMs_ = nowMs;
    confirmedKnown_ = false;
    state_ = SalonKnxState::Pending;
  }

  // A bus status is authoritative, including when produced by another KNX device.
  constexpr void receiveStatus(bool value) {
    confirmedKnown_ = true;
    confirmedValue_ = value;
    lastConfirmedKnown_ = true;
    lastConfirmedValue_ = value;
    state_ = value ? SalonKnxState::On : SalonKnxState::Off;
  }

  constexpr bool expirePending(uint32_t nowMs,
                               uint32_t timeoutMs = kDefaultFeedbackTimeoutMs) {
    if (state_ != SalonKnxState::Pending || nowMs - pendingSinceMs_ < timeoutMs) {
      return false;
    }
    state_ = SalonKnxState::Unknown;
    confirmedKnown_ = false;
    return true;
  }

  constexpr bool requestedValue() const { return requestedValue_; }

 private:
  SalonKnxState state_ = SalonKnxState::Unknown;
  bool confirmedKnown_ = false;
  bool confirmedValue_ = false;
  bool lastConfirmedKnown_ = false;
  bool lastConfirmedValue_ = false;
  bool requestedValue_ = false;
  uint32_t pendingSinceMs_ = 0;
};

