#include "one_shot_authorization.h"
#include "touch_release_gate.h"

namespace {
constexpr bool testOneShotAuthorization() {
  OneShotAuthorization gate;
  if (gate.armed() || gate.consumed() || gate.armOnce(false) || gate.consume()) return false;
  if (!gate.armOnce(true) || !gate.armed() || !gate.consume()) return false;
  return gate.consumed() && !gate.armed() && !gate.consume() && !gate.armOnce(true);
}

constexpr bool testTouchReleaseDebounce() {
  TouchReleaseGate gate;
  gate.capture(0);
  if (gate.complete(0) != 0) return false;
  if (gate.complete(0) != -1) return false;  // repeated release cannot retrigger
  gate.capture(0);
  if (gate.complete(1) != -1) return false;  // slide cancels
  gate.capture(-1);
  return gate.complete(-1) == -1;
}

static_assert(testOneShotAuthorization(), "authorization must be explicit and single use");
static_assert(testTouchReleaseDebounce(), "one physical gesture must yield at most one action");
}  // namespace

