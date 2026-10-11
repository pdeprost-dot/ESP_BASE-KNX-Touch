#pragma once

#include <stdint.h>
#include <stddef.h>

struct KnxTouchButtonConfig {
  static constexpr size_t kNameCapacity = 24;
  char name[kNameCapacity]{};
  uint16_t commandAddress = 0;
  uint16_t statusAddress = 0;
  uint8_t enabled = 0;
};

enum class KnxConfiguredTransport : uint8_t { Routing = 1 };
enum class KnxSynchronizationMode : uint8_t { Passive = 1 };
enum class KnxOperatingMode : uint8_t { Passive = 1 };

struct KnxGeneralConfig {
  uint16_t individualAddress = 0;
  uint16_t port = 3671;
  uint32_t routerIpv4 = 0;
  uint32_t multicastIpv4 = 0;
  uint32_t feedbackTimeoutMs = 5000;
  KnxConfiguredTransport transport = KnxConfiguredTransport::Routing;
  KnxSynchronizationMode synchronization = KnxSynchronizationMode::Passive;
  KnxOperatingMode operatingMode = KnxOperatingMode::Passive;
  uint8_t reserved = 0;
};

struct KnxTouchConfigData {
  uint32_t magic = 0;
  uint16_t schemaVersion = 0;
  uint16_t size = 0;
  KnxGeneralConfig general{};
  KnxTouchButtonConfig buttons[4]{};
  uint32_t checksum = 0;
};

class KnxTouchConfigStore {
 public:
  static constexpr uint32_t kMagic = 0x4B545343;  // KTSC
  static constexpr uint16_t kSchemaVersion = 2;

  bool begin();
  bool load();
  bool save(const KnxTouchConfigData& candidate);
  const KnxTouchConfigData& data() const { return data_; }

  static KnxTouchConfigData defaults();
  static bool parseGroupAddress(const char* text, uint16_t& address);
  static void formatGroupAddress(uint16_t address, char* output, size_t capacity);
  static bool parseIndividualAddress(const char* text, uint16_t& address);
  static void formatIndividualAddress(uint16_t address, char* output, size_t capacity);
  static bool parseIpv4(const char* text, uint32_t& address, bool optional = false);
  static void formatIpv4(uint32_t address, char* output, size_t capacity);
  static bool validate(const KnxTouchConfigData& value);
  static uint32_t calculateChecksum(const KnxTouchConfigData& value);
  static void finalize(KnxTouchConfigData& value);
  static bool migrateLegacyV1(const void* bytes, size_t length,
                              KnxTouchConfigData& migrated);

 private:
  KnxTouchConfigData data_{};
  bool ready_ = false;
};

