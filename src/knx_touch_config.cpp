#include "knx_touch_config.h"

#include <Arduino.h>
#include <Preferences.h>
#include <cstring>

namespace {
constexpr char kNamespace[] = "knx-touch";
constexpr char kKey[] = "config";

struct LegacyConfigV1 {
  uint32_t magic = 0;
  uint16_t schemaVersion = 0;
  uint16_t size = 0;
  KnxTouchButtonConfig buttons[4]{};
  uint32_t checksum = 0;
};

uint32_t checksumBytes(const void* data, size_t length) {
  const uint8_t* bytes = static_cast<const uint8_t*>(data);
  uint32_t hash = 2166136261UL;
  for (size_t i = 0; i < length; ++i) {
    hash ^= bytes[i];
    hash *= 16777619UL;
  }
  return hash;
}

bool validName(const char* name) {
  if (!name) return false;
  const size_t length = strnlen(name, KnxTouchButtonConfig::kNameCapacity);
  if (length == 0 || length >= KnxTouchButtonConfig::kNameCapacity) return false;
  for (size_t i = 0; i < length; ++i) {
    const uint8_t value = static_cast<uint8_t>(name[i]);
    if (value < 0x20 || value == 0x7F || value == '<' || value == '>') return false;
  }
  return true;
}

void setButton(KnxTouchButtonConfig& button, const char* name,
               uint16_t command, uint16_t status) {
  strlcpy(button.name, name, sizeof(button.name));
  button.commandAddress = command;
  button.statusAddress = status;
  button.enabled = 0;
}
}  // namespace

KnxTouchConfigData KnxTouchConfigStore::defaults() {
  KnxTouchConfigData value{};
  value.general.individualAddress = 0xFFFA;      // 15.15.250; must be reviewed.
  value.general.port = 3671;
  value.general.routerIpv4 = 0;                  // Optional in Routing.
  value.general.multicastIpv4 = 0xE000170C;      // 224.0.23.12
  value.general.feedbackTimeoutMs = 5000;
  setButton(value.buttons[0], "Button 1", 0x0001, 0x0101);
  setButton(value.buttons[1], "Button 2", 0x0002, 0x0102);
  setButton(value.buttons[2], "Button 3", 0x0003, 0x0103);
  setButton(value.buttons[3], "Button 4", 0x0004, 0x0104);
  finalize(value);
  return value;
}

bool KnxTouchConfigStore::parseIndividualAddress(const char* text, uint16_t& address) {
  if (!text || !*text) return false;
  uint32_t area = 0, line = 0, device = 0;
  char trailing = 0;
  if (sscanf(text, "%lu.%lu.%lu%c", &area, &line, &device, &trailing) != 3 ||
      area > 15 || line > 15 || device == 0 || device > 255) return false;
  address = static_cast<uint16_t>((area << 12) | (line << 8) | device);
  return true;
}

void KnxTouchConfigStore::formatIndividualAddress(uint16_t address, char* output,
                                                   size_t capacity) {
  if (!output || capacity == 0) return;
  snprintf(output, capacity, "%u.%u.%u", (address >> 12) & 0x0F,
           (address >> 8) & 0x0F, address & 0xFF);
}

bool KnxTouchConfigStore::parseIpv4(const char* text, uint32_t& address, bool optional) {
  if (!text || !*text) {
    if (!optional) return false;
    address = 0;
    return true;
  }
  uint32_t a = 0, b = 0, c = 0, d = 0;
  char trailing = 0;
  if (sscanf(text, "%lu.%lu.%lu.%lu%c", &a, &b, &c, &d, &trailing) != 4 ||
      a > 255 || b > 255 || c > 255 || d > 255) return false;
  address = (a << 24) | (b << 16) | (c << 8) | d;
  return true;
}

void KnxTouchConfigStore::formatIpv4(uint32_t address, char* output, size_t capacity) {
  if (!output || capacity == 0) return;
  if (address == 0) {
    output[0] = '\0';
    return;
  }
  snprintf(output, capacity, "%lu.%lu.%lu.%lu",
           static_cast<unsigned long>((address >> 24) & 0xFF),
           static_cast<unsigned long>((address >> 16) & 0xFF),
           static_cast<unsigned long>((address >> 8) & 0xFF),
           static_cast<unsigned long>(address & 0xFF));
}

bool KnxTouchConfigStore::parseGroupAddress(const char* text, uint16_t& address) {
  if (!text || !*text) return false;
  uint32_t main = 0, middle = 0, sub = 0;
  char trailing = 0;
  if (sscanf(text, "%lu/%lu/%lu%c", &main, &middle, &sub, &trailing) != 3 ||
      main > 31 || middle > 7 || sub > 255) return false;
  const uint16_t encoded = static_cast<uint16_t>((main << 11) | (middle << 8) | sub);
  if (encoded == 0) return false;
  address = encoded;
  return true;
}

void KnxTouchConfigStore::formatGroupAddress(uint16_t address, char* output,
                                              size_t capacity) {
  if (!output || capacity == 0) return;
  snprintf(output, capacity, "%u/%u/%u", (address >> 11) & 0x1F,
           (address >> 8) & 0x07, address & 0xFF);
}

uint32_t KnxTouchConfigStore::calculateChecksum(const KnxTouchConfigData& value) {
  return checksumBytes(&value, offsetof(KnxTouchConfigData, checksum));
}

void KnxTouchConfigStore::finalize(KnxTouchConfigData& value) {
  value.magic = kMagic;
  value.schemaVersion = kSchemaVersion;
  value.size = sizeof(KnxTouchConfigData);
  value.checksum = calculateChecksum(value);
}

bool KnxTouchConfigStore::migrateLegacyV1(const void* bytes, size_t length,
                                           KnxTouchConfigData& migrated) {
  if (!bytes || length != sizeof(LegacyConfigV1)) return false;
  const auto& legacy = *static_cast<const LegacyConfigV1*>(bytes);
  if (legacy.magic != kMagic || legacy.schemaVersion != 1 ||
      legacy.size != sizeof(legacy) ||
      legacy.checksum != checksumBytes(&legacy, offsetof(LegacyConfigV1, checksum))) return false;
  migrated = defaults();
  // Schema 1 had no general block; preserve its historical runtime address.
  migrated.general.individualAddress = 0x11FA;
  memcpy(migrated.buttons, legacy.buttons, sizeof(migrated.buttons));
  finalize(migrated);
  return validate(migrated);
}

bool KnxTouchConfigStore::validate(const KnxTouchConfigData& value) {
  if (value.magic != kMagic || value.schemaVersion != kSchemaVersion ||
      value.size != sizeof(KnxTouchConfigData) ||
      value.checksum != calculateChecksum(value)) return false;
  const auto& general = value.general;
  const uint8_t multicastFirst = static_cast<uint8_t>(general.multicastIpv4 >> 24);
  if ((general.individualAddress & 0xFF) == 0 || general.port == 0 ||
      multicastFirst < 224 || multicastFirst > 239 ||
      general.feedbackTimeoutMs < 1000 || general.feedbackTimeoutMs > 60000 ||
      general.transport != KnxConfiguredTransport::Routing ||
      general.synchronization != KnxSynchronizationMode::Passive ||
      general.operatingMode != KnxOperatingMode::Passive || general.reserved != 0) return false;
  for (const auto& button : value.buttons) {
    if (!validName(button.name) || button.enabled > 1 ||
        button.commandAddress == 0 || button.statusAddress == 0 ||
        button.commandAddress == button.statusAddress) return false;
  }
  for (size_t left = 0; left < 4; ++left) {
    for (size_t right = left + 1; right < 4; ++right) {
      if (value.buttons[left].commandAddress == value.buttons[right].commandAddress ||
          value.buttons[left].statusAddress == value.buttons[right].statusAddress) return false;
    }
  }
  return true;
}

bool KnxTouchConfigStore::begin() {
  ready_ = true;
  data_ = defaults();
  return true;
}

bool KnxTouchConfigStore::load() {
  if (!ready_) return false;
  Preferences preferences;
  if (!preferences.begin(kNamespace, true)) return false;
  const size_t storedSize = preferences.getBytesLength(kKey);
  if (storedSize == sizeof(KnxTouchConfigData)) {
    KnxTouchConfigData candidate{};
    const size_t read = preferences.getBytes(kKey, &candidate, sizeof(candidate));
    preferences.end();
    if (read != sizeof(candidate) || !validate(candidate)) return false;
    data_ = candidate;
    return true;
  }
  if (storedSize == sizeof(LegacyConfigV1)) {
    LegacyConfigV1 legacy{};
    const size_t read = preferences.getBytes(kKey, &legacy, sizeof(legacy));
    preferences.end();
    KnxTouchConfigData migrated{};
    if (read != sizeof(legacy) || !migrateLegacyV1(&legacy, sizeof(legacy), migrated)) return false;
    data_ = migrated;
    return true;
  }
  preferences.end();
  return false;
}

bool KnxTouchConfigStore::save(const KnxTouchConfigData& candidate) {
  if (!ready_ || !validate(candidate)) return false;
  if (memcmp(&candidate, &data_, sizeof(candidate)) == 0) return true;
  Preferences preferences;
  if (!preferences.begin(kNamespace, false)) return false;
  const size_t written = preferences.putBytes(kKey, &candidate, sizeof(candidate));
  preferences.end();
  if (written != sizeof(candidate)) return false;
  data_ = candidate;
  return true;
}

