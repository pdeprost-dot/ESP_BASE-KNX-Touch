#include "knx_runtime_control.h"

namespace {
constexpr bool runtimeModeIsSafe() {
  KnxRuntimeControl control;
  if (control.active() || control.mayTransmit(true, true, false)) return false;
  control.setActive(true);
  if (!control.active() || !control.mayTransmit(true, true, false)) return false;
  if (control.mayTransmit(false, true, false) ||
      control.mayTransmit(true, false, false) ||
      control.mayTransmit(true, true, true)) return false;
  control.forcePassive();
  return !control.active() && !control.mayTransmit(true, true, false) &&
         control.generation() == 2;
}

static_assert(runtimeModeIsSafe(),
              "ACTIVE must be explicit and gated by button, network and state");
}  // namespace
