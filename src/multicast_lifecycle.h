#pragma once

#include <stdint.h>

enum class MulticastNetworkAction : uint8_t {
  None,
  NetworkLost,
  NetworkChanged,
  JoinReady,
};

class MulticastLifecycle {
 public:
  static constexpr uint32_t kDefaultStabilityMs = 1500;

  constexpr MulticastNetworkAction observe(bool wifiConnected, uint32_t localIp,
                                            uint32_t nowMs,
                                            uint32_t stabilityMs = kDefaultStabilityMs) {
    const bool usable = wifiConnected && localIp != 0;
    if (!usable) {
      const bool hadNetwork = networkPresent_;
      networkPresent_ = false;
      joinPending_ = true;
      localIp_ = 0;
      return hadNetwork ? MulticastNetworkAction::NetworkLost
                        : MulticastNetworkAction::None;
    }

    if (!networkPresent_ || localIp != localIp_) {
      networkPresent_ = true;
      localIp_ = localIp;
      stableSinceMs_ = nowMs;
      joinPending_ = true;
      return MulticastNetworkAction::NetworkChanged;
    }

    if (joinPending_ && nowMs - stableSinceMs_ >= stabilityMs) {
      joinPending_ = false;
      return MulticastNetworkAction::JoinReady;
    }
    return MulticastNetworkAction::None;
  }

  constexpr uint32_t localIp() const { return localIp_; }
  constexpr bool joinPending() const { return joinPending_; }

 private:
  bool networkPresent_ = false;
  bool joinPending_ = true;
  uint32_t localIp_ = 0;
  uint32_t stableSinceMs_ = 0;
};

