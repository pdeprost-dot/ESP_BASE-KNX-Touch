#include "knx_touch_config.h"

#include <cstdlib>
#include <cstring>

namespace {
struct LegacyConfigV1ForTest {
  uint32_t magic = 0;
  uint16_t schemaVersion = 0;
  uint16_t size = 0;
  KnxTouchButtonConfig buttons[4]{};
  uint32_t checksum = 0;
};

uint32_t legacyChecksum(const LegacyConfigV1ForTest& value) {
  const auto* bytes = reinterpret_cast<const uint8_t*>(&value);
  uint32_t hash = 2166136261UL;
  for (size_t i = 0; i < offsetof(LegacyConfigV1ForTest, checksum); ++i) {
    hash ^= bytes[i];
    hash *= 16777619UL;
  }
  return hash;
}

bool configTests() {
  uint16_t address = 0;
  if (!KnxTouchConfigStore::parseGroupAddress("31/7/255", address) ||
      address != 0xFFFF) return false;
  if (KnxTouchConfigStore::parseGroupAddress("32/0/1", address) ||
      KnxTouchConfigStore::parseGroupAddress("1/8/1", address) ||
      KnxTouchConfigStore::parseGroupAddress("1/1/256", address) ||
      KnxTouchConfigStore::parseGroupAddress("1/1/1x", address) ||
      KnxTouchConfigStore::parseGroupAddress("0/0/0", address)) return false;
  KnxTouchConfigData value = KnxTouchConfigStore::defaults();
  if (!KnxTouchConfigStore::validate(value)) return false;
  if (value.buttons[0].commandAddress == value.buttons[1].commandAddress) return false;
  value.buttons[2].statusAddress = value.buttons[2].commandAddress;
  KnxTouchConfigStore::finalize(value);
  if (KnxTouchConfigStore::validate(value)) return false;
  value = KnxTouchConfigStore::defaults();
  value.buttons[3].statusAddress = value.buttons[1].statusAddress;
  KnxTouchConfigStore::finalize(value);
  if (KnxTouchConfigStore::validate(value)) return false;
  value = KnxTouchConfigStore::defaults();
  value.checksum ^= 1;
  if (KnxTouchConfigStore::validate(value)) return false;

  uint16_t individual = 0;
  uint32_t ipv4 = 0;
  if (!KnxTouchConfigStore::parseIndividualAddress("1.1.250", individual) ||
      individual != 0x11FA ||
      KnxTouchConfigStore::parseIndividualAddress("16.1.1", individual) ||
      KnxTouchConfigStore::parseIndividualAddress("1.1.0", individual) ||
      !KnxTouchConfigStore::parseIpv4("224.0.23.12", ipv4) ||
      ipv4 != 0xE000170C || KnxTouchConfigStore::parseIpv4("256.0.0.1", ipv4)) return false;

  LegacyConfigV1ForTest legacy{};
  const KnxTouchConfigData defaults = KnxTouchConfigStore::defaults();
  legacy.magic = KnxTouchConfigStore::kMagic;
  legacy.schemaVersion = 1;
  legacy.size = sizeof(legacy);
  memcpy(legacy.buttons, defaults.buttons, sizeof(legacy.buttons));
  legacy.buttons[0].enabled = 1;
  legacy.checksum = legacyChecksum(legacy);
  KnxTouchConfigData migrated{};
  if (!KnxTouchConfigStore::migrateLegacyV1(&legacy, sizeof(legacy), migrated) ||
      migrated.schemaVersion != KnxTouchConfigStore::kSchemaVersion ||
      migrated.general.individualAddress != 0x11FA || !migrated.buttons[0].enabled) return false;
  legacy.checksum ^= 1;
  return !KnxTouchConfigStore::migrateLegacyV1(&legacy, sizeof(legacy), migrated);
}

struct RunConfigTests {
  RunConfigTests() { if (!configTests()) std::abort(); }
} runConfigTests;
}  // namespace

