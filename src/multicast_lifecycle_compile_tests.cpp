#include "multicast_lifecycle.h"

namespace {
constexpr bool testColdStartAndSilence() {
  MulticastLifecycle lifecycle;
  if (lifecycle.observe(false, 0, 0) != MulticastNetworkAction::None) return false;
  if (lifecycle.observe(true, 0xC0A832F5, 100) !=
      MulticastNetworkAction::NetworkChanged) return false;
  if (lifecycle.observe(true, 0xC0A832F5, 1599) !=
      MulticastNetworkAction::None) return false;
  if (lifecycle.observe(true, 0xC0A832F5, 1600) !=
      MulticastNetworkAction::JoinReady) return false;
  // Arbitrarily long traffic silence must never cause a blind rejoin.
  return lifecycle.observe(true, 0xC0A832F5, 3600000) ==
         MulticastNetworkAction::None;
}

constexpr bool testDisconnectReconnectAndIpChange() {
  MulticastLifecycle lifecycle;
  lifecycle.observe(true, 0xC0A832F5, 0);
  lifecycle.observe(true, 0xC0A832F5, 1500);
  if (lifecycle.observe(false, 0, 2000) != MulticastNetworkAction::NetworkLost) return false;
  if (lifecycle.observe(false, 0, 2500) != MulticastNetworkAction::None) return false;
  if (lifecycle.observe(true, 0xC0A832F5, 3000) !=
      MulticastNetworkAction::NetworkChanged) return false;
  if (lifecycle.observe(true, 0xC0A832F5, 4500) !=
      MulticastNetworkAction::JoinReady) return false;
  if (lifecycle.observe(true, 0xC0A832F6, 5000) !=
      MulticastNetworkAction::NetworkChanged) return false;
  return lifecycle.observe(true, 0xC0A832F6, 6500) ==
         MulticastNetworkAction::JoinReady;
}

static_assert(testColdStartAndSilence(), "silence must not trigger multicast recovery");
static_assert(testDisconnectReconnectAndIpChange(),
              "network transitions must renew multicast membership");
}  // namespace

