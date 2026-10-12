#include "salon_knx_model.h"
#include "knx_status_dispatch.h"

namespace {
constexpr bool independentModels() {
  SalonKnxModel models[4];
  models[2].receiveStatus(true);
  return models[0].state() == SalonKnxState::Unknown &&
         models[1].state() == SalonKnxState::Unknown &&
         models[2].state() == SalonKnxState::On &&
         models[3].state() == SalonKnxState::Unknown;
}

constexpr bool blockedIntentDoesNotBecomePending() {
  SalonKnxModel model;
  bool requested = false;
  return model.prepareCommand(requested) && requested &&
         model.state() == SalonKnxState::Unknown;
}

constexpr bool sharedStatusUpdatesEveryActiveSubscriber() {
  KnxTouchButtonConfig buttons[4]{};
  SalonKnxModel models[4];
  for (size_t index = 0; index < 4; ++index) {
    buttons[index].enabled = 1;
    buttons[index].commandAddress = 0x1000;
    buttons[index].statusAddress = 0x1100;
  }
  const uint8_t updated = applyKnxStatusToButtons(buttons, models, 4, 0x1100, true);
  return updated == 0x0F && models[0].state() == SalonKnxState::On &&
         models[1].state() == SalonKnxState::On &&
         models[2].state() == SalonKnxState::On &&
         models[3].state() == SalonKnxState::On;
}

constexpr bool sharedCommandDoesNotCoupleDifferentStatuses() {
  KnxTouchButtonConfig buttons[4]{};
  SalonKnxModel models[4];
  for (size_t index = 0; index < 4; ++index) {
    buttons[index].enabled = 1;
    buttons[index].commandAddress = 0x1000;
    buttons[index].statusAddress = static_cast<uint16_t>(0x1100 + index);
  }
  const uint8_t updated = applyKnxStatusToButtons(buttons, models, 4, 0x1102, false);
  return updated == 0x04 && models[0].state() == SalonKnxState::Unknown &&
         models[1].state() == SalonKnxState::Unknown &&
         models[2].state() == SalonKnxState::Off &&
         models[3].state() == SalonKnxState::Unknown;
}

constexpr bool disabledSubscriberIsNotUpdated() {
  KnxTouchButtonConfig buttons[2]{};
  SalonKnxModel models[2];
  buttons[0].enabled = 1;
  buttons[1].enabled = 0;
  buttons[0].statusAddress = buttons[1].statusAddress = 0x1100;
  return applyKnxStatusToButtons(buttons, models, 2, 0x1100, true) == 0x01 &&
         models[0].state() == SalonKnxState::On &&
         models[1].state() == SalonKnxState::Unknown;
}

static_assert(independentModels(), "Button models must retain independent state");
static_assert(blockedIntentDoesNotBecomePending(),
              "Preparing a blocked command must not enter PENDING");
static_assert(sharedStatusUpdatesEveryActiveSubscriber(),
              "A shared status must update every active subscriber");
static_assert(sharedCommandDoesNotCoupleDifferentStatuses(),
              "A shared command must not couple independent status groups");
static_assert(disabledSubscriberIsNotUpdated(),
              "Disabled buttons must ignore shared status groups");
}  // namespace
