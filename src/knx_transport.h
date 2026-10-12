#pragma once

#include <Arduino.h>
#include <WiFi.h>
#include <WiFiUdp.h>

#include "knx_cemi_codec.h"
#include "multicast_lifecycle.h"

enum class KnxTransportState : uint8_t {
  Disabled,
  WaitingForWifi,
  Disconnected,
  Connecting,
  Connected,
  Error,
};

enum class KnxTransportMode : uint8_t {
  Tunneling,
  Routing,
};

struct KnxTelegram {
  uint16_t source = 0;
  uint16_t destination = 0;
  bool groupDestination = false;
  KnxApci apci = KnxApci::Other;
  bool hasInlineValue = false;
  bool inlineValue = false;
};

class KNXTransport {
 public:
  struct Config {
    bool enabled = false;
    KnxTransportMode mode = KnxTransportMode::Tunneling;
    IPAddress serverIp;
    uint16_t serverPort = 3671;
    uint16_t localPort = 3672;
    IPAddress multicastIp = IPAddress(224, 0, 23, 12);
    // Reserved for a separately authorized transmit phase. Zero means unset.
    uint16_t individualAddress = 0;
    bool applicationTransmitEnabled = false;
    uint16_t authorizedGroupAddress = 0;
    bool groupValueWriteAuthorized = false;
    bool groupValueReadAuthorized = false;
  };

  struct Statistics {
    uint32_t packetsReceived = 0;
    uint32_t routingIndications = 0;
    uint32_t validTelegrams = 0;
    uint32_t groupTelegrams = 0;
    uint32_t individualTelegrams = 0;
    uint32_t otherKnxPackets = 0;
    uint32_t invalidPackets = 0;
    uint32_t wifiLosses = 0;
    uint32_t networkGenerations = 0;
    uint32_t multicastJoinAttempts = 0;
    uint32_t multicastJoinSuccesses = 0;
    uint32_t socketErrors = 0;
  };

  using TelegramHandler = void (*)(const KnxTelegram& telegram);

  void begin(const Config& config, TelegramHandler handler = nullptr);
  void loop();
  void disconnect();
  KnxTransportState state() const;
  const char* stateName() const;
  const char* modeName() const;
  const Statistics& statistics() const;
  uint32_t synchronizationEpoch() const;
  bool applicationNetworkOperational() const;
  bool setApplicationTransmitEnabled(bool enabled,
                                     const uint16_t* authorizedWriteAddresses = nullptr,
                                     size_t count = 0);
  void setKnownDpt1Addresses(const uint16_t* addresses, size_t count);
  bool sendGroupValueWriteDpt1(uint16_t groupAddress, bool value);
  bool sendGroupValueRead(uint16_t groupAddress);

  static bool decodeCemi(const uint8_t* data, size_t length, KnxTelegram& telegram);
  static size_t encodeRoutingGroupCemi(uint8_t* output, size_t capacity,
                                       uint16_t source, uint16_t groupAddress,
                                       KnxApci apci, bool value = false);
  static size_t encodeTunnelingGroupCemi(uint8_t* output, size_t capacity,
                                         uint16_t source, uint16_t groupAddress,
                                         KnxApci apci, bool value = false);
  static size_t buildRoutingIndication(uint8_t* output, size_t capacity,
                                       uint16_t source, uint16_t groupAddress,
                                       KnxApci apci, bool value = false);
  static void formatIndividualAddress(uint16_t address, char* output, size_t length);
  static void formatGroupAddress(uint16_t address, char* output, size_t length);
  static bool runDecoderSelfTest();
  static bool runEncoderSelfTest();

 private:
  static constexpr size_t kMaxPacketSize = 256;
  static constexpr size_t kMaxKnownDpt1 = 16;
  static constexpr size_t kMaxAuthorizedWrites = 4;

  void setState(KnxTransportState state, const char* reason);
  void startConnection();
  void startRouting();
  void sendConnectionRequest();
  void sendConnectionStateRequest();
  void sendDisconnectRequest();
  void sendTunnelingAck(uint8_t channel, uint8_t sequence, IPAddress ip, uint16_t port);
  bool sendPacket(const uint8_t* data, size_t length, IPAddress ip, uint16_t port,
                  size_t* bytesWritten = nullptr);
  void receivePackets();
  void handlePacket(const uint8_t* data, size_t length, IPAddress remoteIp,
                    uint16_t remotePort);
  void handleCemi(const uint8_t* data, size_t length);
  bool isKnownDpt1(uint16_t groupAddress) const;
  bool transmitAuthorized(uint16_t groupAddress, KnxApci apci);
  bool isAuthorizedWriteAddress(uint16_t groupAddress) const;
  bool sendRoutingGroup(uint16_t groupAddress, KnxApci apci, bool value);

  Config config_;
  WiFiUDP udp_;
  TelegramHandler handler_ = nullptr;
  KnxTransportState state_ = KnxTransportState::Disabled;
  uint8_t channelId_ = 0;
  uint8_t expectedSequence_ = 0;
  uint32_t stateSinceMs_ = 0;
  uint32_t lastConnectAttemptMs_ = 0;
  uint32_t lastHeartbeatMs_ = 0;
  uint32_t heartbeatSentMs_ = 0;
  uint32_t lastStatsMs_ = 0;
  bool heartbeatPending_ = false;
  bool udpOpen_ = false;
  bool retryAllowed_ = true;
  uint16_t knownDpt1_[kMaxKnownDpt1]{};
  size_t knownDpt1Count_ = 0;
  uint8_t packet_[kMaxPacketSize]{};
  Statistics statistics_{};
  uint8_t ignoredServiceLogs_ = 0;
  uint16_t authorizedWriteAddresses_[kMaxAuthorizedWrites]{};
  size_t authorizedWriteAddressCount_ = 0;
  MulticastLifecycle multicastLifecycle_;
  uint32_t synchronizationEpoch_ = 0;
};

