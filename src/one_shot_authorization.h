#pragma once

class OneShotAuthorization {
 public:
  constexpr bool armed() const { return armed_; }
  constexpr bool consumed() const { return consumed_; }

  constexpr bool armOnce(bool explicitAuthorization) {
    if (!explicitAuthorization || consumed_) return false;
    armed_ = true;
    return true;
  }

  // Consumption happens before attempting network I/O and is irreversible.
  constexpr bool consume() {
    if (!armed_ || consumed_) return false;
    armed_ = false;
    consumed_ = true;
    return true;
  }

 private:
  bool armed_ = false;
  bool consumed_ = false;
};

