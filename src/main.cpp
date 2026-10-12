#include <Arduino.h>
#include <ESPBase.h>

#include "four_button_ui.h"
#include "knx_touch_config.h"
#include "knx_touch_web.h"
#include "knx_status_dispatch.h"
#include "knx_runtime_control.h"
#include "knx_transport.h"
#include "lcd_driver.h"
#include "salon_knx_model.h"
#include "touch_driver.h"

namespace {
constexpr char kProjectName[] = "ESP_BASE-KNX-Touch";
constexpr char kFirmwareVersion[] = "0.2.0-alpha";
constexpr size_t kButtonCount = 4;

ESPBase espBase(kProjectName, kFirmwareVersion);
LcdDriver lcd;
TouchDriver touch;
FourButtonUi ui;
KNXTransport knxTransport;
KnxTouchConfigStore touchConfigStore;
KnxTouchWeb touchWeb;
SalonKnxModel buttonModels[kButtonCount];
KnxRuntimeControl runtimeControl;
bool uiReady = false;
uint32_t observedSynchronizationEpoch = 0;

IPAddress toIpAddress(uint32_t value) {
  return IPAddress((value >> 24) & 0xFF, (value >> 16) & 0xFF,
                   (value >> 8) & 0xFF, value & 0xFF);
}

ButtonState uiState(size_t index) {
  switch (buttonModels[index].state()) {
    case SalonKnxState::On: return ButtonState::On;
    case SalonKnxState::Off: return ButtonState::Off;
    case SalonKnxState::Pending: return ButtonState::Pending;
    case SalonKnxState::Unknown: return ButtonState::Unknown;
  }
  return ButtonState::Unknown;
}

void refreshUi(size_t index) {
  if (uiReady && index < kButtonCount) ui.setButtonState(index, uiState(index));
}

void applyTouchConfiguration(void*) {
  runtimeControl.forcePassive();
  knxTransport.setApplicationTransmitEnabled(false);
  uint16_t statusAddresses[kButtonCount]{};
  const auto& config = touchConfigStore.data();
  for (size_t index = 0; index < kButtonCount; ++index) {
    statusAddresses[index] = config.buttons[index].statusAddress;
    buttonModels[index].invalidateSynchronization();
    ui.setButtonName(index, config.buttons[index].name);
    refreshUi(index);
  }
  knxTransport.setKnownDpt1Addresses(statusAddresses, kButtonCount);
  Serial.println("[CONFIG][APPLY] four independent buttons; states=UNKNOWN; transmit=BLOCKED");
}

bool applyRuntimeMode(bool active, bool persist, const char* source) {
  if (!active) {
    runtimeControl.forcePassive();
    knxTransport.setApplicationTransmitEnabled(false);
    if (persist && !touchConfigStore.saveOperatingMode(KnxOperatingMode::Passive)) {
      Serial.println("[BUTTON][SECURITY] PASSIVE persistence failed");
      return false;
    }
    Serial.printf("[BUTTON][SECURITY] runtime mode=PASSIVE source=%s persistent=%s\n",
                  source, persist ? "yes" : "unchanged");
    return true;
  }
  if (!knxTransport.applicationNetworkOperational()) {
    Serial.println("[BUTTON][SECURITY] ACTIVE rejected: multicast unavailable");
    return false;
  }
  uint16_t authorized[kButtonCount]{};
  size_t count = 0;
  for (const auto& button : touchConfigStore.data().buttons) {
    if (button.enabled) authorized[count++] = button.commandAddress;
  }
  if (!knxTransport.setApplicationTransmitEnabled(true, authorized, count)) return false;
  runtimeControl.setActive(true);
  if (persist && !touchConfigStore.saveOperatingMode(KnxOperatingMode::Active)) {
    runtimeControl.forcePassive();
    knxTransport.setApplicationTransmitEnabled(false);
    Serial.println("[BUTTON][SECURITY] ACTIVE persistence failed; reverted to PASSIVE");
    return false;
  }
  Serial.printf("[BUTTON][SECURITY] runtime mode=ACTIVE source=%s persistent=%s\n",
                source, persist ? "yes" : "restored");
  return true;
}

bool setRuntimeMode(bool active, void*) {
  return applyRuntimeMode(active, true, "WEB");
}

bool runtimeModeActive(void*) { return runtimeControl.active(); }

void handleKnxTelegram(const KnxTelegram& telegram) {
  if (!telegram.groupDestination || !telegram.hasInlineValue ||
      (telegram.apci != KnxApci::GroupValueWrite &&
       telegram.apci != KnxApci::GroupValueResponse)) return;
  const auto& config = touchConfigStore.data();
  const uint8_t updated = applyKnxStatusToButtons(
      config.buttons, buttonModels, kButtonCount, telegram.destination,
      telegram.inlineValue);
  for (size_t index = 0; index < kButtonCount; ++index) {
    if ((updated & (1U << index)) == 0) continue;
    refreshUi(index);
    char group[16]{};
    KnxTouchConfigStore::formatGroupAddress(telegram.destination, group, sizeof(group));
    Serial.printf("[BUTTON][STATUS] index=%u name=%s group=%s value=%s state=CONFIRMED\n",
                  static_cast<unsigned>(index), config.buttons[index].name, group,
                  telegram.inlineValue ? "ON" : "OFF");
  }
  if (updated == 0) {
    Serial.printf("[BUTTON][STATUS] foreign_or_disabled_group=0x%04X ignored\n",
                  telegram.destination);
  }
}

void handleButtonAction(uint8_t index) {
  if (index >= kButtonCount) return;
  const auto& button = touchConfigStore.data().buttons[index];
  if (!button.enabled) {
    Serial.printf("[BUTTON][SECURITY] index=%u name=%s blocked=DISABLED\n", index, button.name);
    return;
  }
  if (!runtimeControl.mayTransmit(
          button.enabled, knxTransport.applicationNetworkOperational(),
          buttonModels[index].state() == SalonKnxState::Pending)) {
    Serial.printf("[BUTTON][SECURITY] index=%u name=%s blocked=%s\n", index,
                  button.name, runtimeControl.active() ? "NETWORK_OR_PENDING" : "PASSIVE");
    return;
  }
  bool requestedValue = false;
  if (!buttonModels[index].prepareCommand(requestedValue)) {
    Serial.printf("[BUTTON][SECURITY] index=%u name=%s blocked=PENDING\n", index, button.name);
    return;
  }
  if (!knxTransport.sendGroupValueWriteDpt1(button.commandAddress, requestedValue)) {
    Serial.printf("[BUTTON][ERROR] index=%u name=%s send=REJECTED state=UNCHANGED\n",
                  index, button.name);
    return;
  }
  buttonModels[index].commandAccepted(requestedValue, millis());
  refreshUi(index);
  Serial.printf("[BUTTON][COMMAND] index=%u name=%s value=%s state=PENDING\n",
                index, button.name, requestedValue ? "ON" : "OFF");
}
}  // namespace

void setup() {
  Serial.begin(115200);
  touchConfigStore.begin();
  if (!touchConfigStore.load()) {
    Serial.println("[CONFIG] no valid stored schema; safe disabled defaults selected");
  } else {
    Serial.println("[CONFIG] valid application configuration loaded");
  }
  if (!touchWeb.registerRoutes(espBase, touchConfigStore, applyTouchConfiguration,
                               nullptr, setRuntimeMode, runtimeModeActive)) {
    Serial.println("[WEB][ERROR] KNX / Touches routes unavailable");
  }
  espBase.begin();

  const bool lcdReady = lcd.begin();
  const bool touchReady = touch.begin();
  if (lcdReady && touchReady) {
    uiReady = ui.begin(lcd);
    ui.setButtonActionHandler(handleButtonAction);
  }

  KNXTransport::Config knxConfig;
  const auto& general = touchConfigStore.data().general;
  knxConfig.enabled = true;
  knxConfig.mode = KnxTransportMode::Routing;
  knxConfig.serverIp = toIpAddress(general.routerIpv4);
  knxConfig.serverPort = general.port;
  knxConfig.multicastIp = toIpAddress(general.multicastIpv4);
  knxConfig.individualAddress = general.individualAddress;
  knxConfig.authorizedGroupAddress = 0;
  knxConfig.applicationTransmitEnabled = false;
  knxConfig.groupValueWriteAuthorized = true;
  knxConfig.groupValueReadAuthorized = false;
  knxTransport.begin(knxConfig, handleKnxTelegram);
  applyTouchConfiguration(nullptr);
  observedSynchronizationEpoch = knxTransport.synchronizationEpoch();
}

void loop() {
  espBase.loop();
  TouchSample sample{};
  if (uiReady && touch.poll(sample)) ui.process(sample);
  knxTransport.loop();

  const uint32_t synchronizationEpoch = knxTransport.synchronizationEpoch();
  if (synchronizationEpoch != observedSynchronizationEpoch) {
    observedSynchronizationEpoch = synchronizationEpoch;
    runtimeControl.forcePassive();
    knxTransport.setApplicationTransmitEnabled(false);
    for (size_t index = 0; index < kButtonCount; ++index) {
      buttonModels[index].invalidateSynchronization();
      refreshUi(index);
    }
    Serial.printf("[BUTTON][SYNC] all states invalidated epoch=%lu state=UNKNOWN\n",
                  static_cast<unsigned long>(synchronizationEpoch));
  }
  const bool configuredActive =
      touchConfigStore.data().general.operatingMode == KnxOperatingMode::Active;
  if (configuredActive && !runtimeControl.active() &&
      knxTransport.applicationNetworkOperational()) {
    applyRuntimeMode(true, false, "RESTORE");
  }
  const uint32_t now = millis();
  for (size_t index = 0; index < kButtonCount; ++index) {
    if (buttonModels[index].expirePending(
            now, touchConfigStore.data().general.feedbackTimeoutMs)) {
      refreshUi(index);
      Serial.printf("[BUTTON][TIMEOUT] index=%u feedback missing; state=UNKNOWN\n",
                    static_cast<unsigned>(index));
    }
  }
}
