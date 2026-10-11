#include "salon_knx_model.h"

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

static_assert(independentModels(), "A status must update only one button model");
static_assert(blockedIntentDoesNotBecomePending(),
              "Preparing a blocked command must not enter PENDING");
}  // namespace
