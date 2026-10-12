#pragma once

#include <stddef.h>
#include <stdint.h>

#include "knx_touch_config.h"
#include "salon_knx_model.h"

// Applies one authoritative KNX status to every active button subscribed to
// the group. The returned bit mask identifies the models that were updated.
constexpr uint8_t applyKnxStatusToButtons(
    const KnxTouchButtonConfig* buttons, SalonKnxModel* models, size_t count,
    uint16_t statusAddress, bool value) {
  uint8_t updated = 0;
  for (size_t index = 0; index < count && index < 8; ++index) {
    if (!buttons[index].enabled ||
        buttons[index].statusAddress != statusAddress) continue;
    models[index].receiveStatus(value);
    updated |= static_cast<uint8_t>(1U << index);
  }
  return updated;
}
