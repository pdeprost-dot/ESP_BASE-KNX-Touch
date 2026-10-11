#include <Arduino.h>
#include <ESPBase.h>

#include "four_button_ui.h"
#include "knx_touch_config.h"
#include "knx_touch_web.h"
#include "knx_transport.h"
#include "lcd_driver.h"
#include "salon_knx_model.h"
#include "touch_driver.h"

namespace {
constexpr char kProjectName[] = "ESP_BASE-KNX-Touch";
constexpr char kFirmwareVersion[] = "0.1.0-alpha";
constexpr size_t kButtonCount = 4;
constexpr bool kApplicationTransmitEnabled = false;

ESPBase espBase(kProjectName, kFirmwareVersion);
LcdDriver lcd;
TouchDriver touch;
FourButtonUi ui;
KNXTransport knxTransport;
KnxTouchConfigStore touchConfigStore;
KnxTouchWeb touchWeb;
SalonKnxModel buttonModels[kButtonCount];
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

void handleKnxTelegram(const KnxTelegram& telegram) {
  if (!telegram.groupDestination || !telegram.hasInlineValue ||
      (telegram.apci != KnxApci::GroupValueWrite &&
       telegram.apci != KnxApci::GroupValueResponse)) return;
  const auto& config = touchConfigStore.data();
  for (size_t index = 0; index < kButtonCount; ++index) {
    if (telegram.destination != config.buttons[index].statusAddress) continue;
    buttonModels[index].receiveStatus(telegram.inlineValue);
    refreshUi(index);
    char group[16]{};
    KnxTouchConfigStore::formatGroupAddress(telegram.destination, group, sizeof(group));
    Serial.printf("[BUTTON][STATUS] index=%u name=%s group=%s value=%s state=CONFIRMED\n",
                  static_cast<unsigned>(index), config.buttons[index].name, group,
                  telegram.inlineValue ? "ON" : "OFF");
    return;
  }
  Serial.printf("[BUTTON][STATUS] foreign_group=0x%04X ignored\n", telegram.destination);
}

void handleButtonAction(uint8_t index) {
  if (index >= kButtonCount) return;
  const auto& button = touchConfigStore.data().buttons[index];
  if (!button.enabled) {
    Serial.printf("[BUTTON][SECURITY] index=%u name=%s blocked=DISABLED\n", index, button.name);
    return;
  }
  bool requestedValue = false;
  if (!buttonModels[index].prepareCommand(requestedValue)) {
    Serial.printf("[BUTTON][SECURITY] index=%u name=%s blocked=PENDING\n", index, button.name);
    return;
  }
  if (!kApplicationTransmitEnabled) {
    Serial.printf("[BUTTON][SECURITY] index=%u name=%s prepared=%s transmit=BLOCKED\n",
                  index, button.name, requestedValue ? "ON" : "OFF");
    return;
  }
  // Intentionally unreachable in Phase 6A. A later phase must add a separately
  // reviewed authorization before invoking the transport.
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
  if (!touchWeb.registerRoutes(espBase, touchConfigStore, applyTouchConfiguration)) {
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
  knxConfig.groupValueWriteAuthorized = false;
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
    for (size_t index = 0; index < kButtonCount; ++index) {
      buttonModels[index].invalidateSynchronization();
      refreshUi(index);
    }
    Serial.printf("[BUTTON][SYNC] all states invalidated epoch=%lu state=UNKNOWN\n",
                  static_cast<unsigned long>(synchronizationEpoch));
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
