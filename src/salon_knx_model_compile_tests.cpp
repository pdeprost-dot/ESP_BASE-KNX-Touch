#include "salon_knx_model.h"

namespace {
constexpr bool testInitialAndExternalStatus() {
  SalonKnxModel model;
  if (model.state() != SalonKnxState::Unknown || model.hasConfirmedState()) return false;
  bool requested = false;
  if (!model.prepareCommand(requested) || !requested) return false;
  model.receiveStatus(true);
  if (model.state() != SalonKnxState::On || !model.prepareCommand(requested) || requested) return false;
  model.receiveStatus(false);
  return model.state() == SalonKnxState::Off && model.prepareCommand(requested) && requested;
}

constexpr bool testPendingIsNotConfirmation() {
  SalonKnxModel model;
  model.receiveStatus(false);
  model.commandAccepted(true, 100);
  if (model.state() != SalonKnxState::Pending || model.hasConfirmedState() ||
      !model.hasLastConfirmedValue() || model.lastConfirmedValue()) return false;
  bool requested = false;
  if (model.prepareCommand(requested)) return false;
  model.receiveStatus(true);
  return model.state() == SalonKnxState::On && model.hasConfirmedState();
}

constexpr bool testMissingFeedbackBecomesUnknown() {
  SalonKnxModel model;
  model.receiveStatus(false);
  model.commandAccepted(true, 0xFFFFFF00U);
  if (model.expirePending(0x00000050U, 500)) return false;
  if (!model.expirePending(0x00000150U, 500) ||
      model.state() != SalonKnxState::Unknown || model.hasConfirmedState()) return false;
  bool requested = false;
  return model.hasLastConfirmedValue() && !model.lastConfirmedValue() &&
         model.prepareCommand(requested) && requested;
}

constexpr bool testNetworkInvalidationAndSpontaneousStatus() {
  SalonKnxModel model;
  model.receiveStatus(true);
  model.invalidateSynchronization();
  if (model.state() != SalonKnxState::Unknown || model.hasConfirmedState() ||
      !model.hasLastConfirmedValue() || !model.lastConfirmedValue()) return false;
  bool requested = false;
  if (!model.prepareCommand(requested) || !requested) return false;
  model.receiveStatus(false);
  return model.state() == SalonKnxState::Off && model.hasConfirmedState() &&
         model.prepareCommand(requested) && requested;
}

constexpr bool testBlockedCommandDoesNotBecomePending() {
  SalonKnxModel model;
  bool requested = false;
  if (!model.prepareCommand(requested) || !requested) return false;
  // A rejected authorization/transport call deliberately omits commandAccepted().
  return model.state() == SalonKnxState::Unknown && !model.hasConfirmedState();
}

static_assert(testInitialAndExternalStatus(), "external KNX status must drive Salon");
static_assert(testPendingIsNotConfirmation(), "UDP acceptance must remain pending");
static_assert(testMissingFeedbackBecomesUnknown(), "missing feedback must expire safely");
static_assert(testNetworkInvalidationAndSpontaneousStatus(),
              "network loss must invalidate and spontaneous status must resynchronize");
static_assert(testBlockedCommandDoesNotBecomePending(),
              "a blocked send must not change the model to pending");
}  // namespace

