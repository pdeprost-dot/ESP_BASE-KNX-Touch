#include "knx_transport.h"

#include <cstring>

namespace {
constexpr uint16_t kConnectRequest = 0x0205;
constexpr uint16_t kConnectResponse = 0x0206;
constexpr uint16_t kConnectionStateRequest = 0x0207;
constexpr uint16_t kConnectionStateResponse = 0x0208;
constexpr uint16_t kDisconnectRequest = 0x0209;
constexpr uint16_t kDisconnectResponse = 0x020A;
constexpr uint16_t kTunnelingRequest = 0x0420;
constexpr uint16_t kTunnelingAck = 0x0421;
constexpr uint16_t kRoutingIndication = 0x0530;
constexpr uint32_t kConnectRetryMs = 10000;
constexpr uint32_t kConnectTimeoutMs = 5000;
constexpr uint32_t kHeartbeatIntervalMs = 60000;
constexpr uint32_t kHeartbeatTimeoutMs = 10000;
constexpr uint32_t kStatsIntervalMs = 60000;
constexpr size_t kRoutingDpt1PacketSize = 17;

void writeHeader(uint8_t* data, uint16_t service, uint16_t length) {
  data[0] = 0x06;
  data[1] = 0x10;
  data[2] = service >> 8;
  data[3] = service & 0xFF;
  data[4] = length >> 8;
  data[5] = length & 0xFF;
}

void writeHpai(uint8_t* output, IPAddress ip, uint16_t port) {
  output[0] = 0x08;
  output[1] = 0x01;
  for (uint8_t i = 0; i < 4; ++i) {
    output[2 + i] = ip[i];
  }
  output[6] = port >> 8;
  output[7] = port & 0xFF;
}

const char* apciName(KnxApci apci) {
  switch (apci) {
    case KnxApci::GroupValueRead: return "GroupValueRead";
    case KnxApci::GroupValueResponse: return "GroupValueResponse";
    case KnxApci::GroupValueWrite: return "GroupValueWrite";
    case KnxApci::Other: return "Other";
  }
  return "Other";
}

const char* connectStatusName(uint8_t status) {
  switch (status) {
    case 0x22: return "unsupported connection type";
    case 0x23: return "unsupported connection option";
    case 0x24: return "no more connections";
    case 0x25: return "no unique tunneling address available";
    case 0x29: return "unsupported tunneling layer";
    default: return "unknown rejection";
  }
}
}  // namespace

void KNXTransport::begin(const Config& config, TelegramHandler handler) {
  config_ = config;
  handler_ = handler;
  transmissionConsumed_ = false;
  const bool validIndividualAddress = config_.individualAddress != 0 &&
                                      (config_.individualAddress & 0x00FF) != 0;
  if (config_.applicationTransmitEnabled &&
      (!validIndividualAddress || config_.authorizedGroupAddress == 0 ||
       (!config_.groupValueWriteAuthorized && !config_.groupValueReadAuthorized))) {
    config_.applicationTransmitEnabled = false;
    Serial.println("[KNX][SECURITY] application transmit forced OFF: incomplete authorization");
  }
  Serial.printf("[KNX] mode=%s decoder_self_test=%s encoder_self_test=%s transmit=%s\n",
                modeName(), runDecoderSelfTest() ? "PASS" : "FAIL",
                runEncoderSelfTest() ? "PASS" : "FAIL",
                config_.applicationTransmitEnabled ? "ONE_SHOT_ARMED" : "BLOCKED");
  if (!config_.enabled ||
      (config_.mode == KnxTransportMode::Tunneling &&
       static_cast<uint32_t>(config_.serverIp) == 0) ||
      (config_.mode == KnxTransportMode::Routing &&
       static_cast<uint32_t>(config_.multicastIp) == 0)) {
    setState(KnxTransportState::Disabled, "no endpoint configured; no KNX packets sent");
    return;
  }
  lastConnectAttemptMs_ = millis() - kConnectRetryMs;
  setState(WiFi.status() == WL_CONNECTED ? KnxTransportState::Disconnected
                                         : KnxTransportState::WaitingForWifi,
           "initialized");
}

void KNXTransport::loop() {
  if (state_ == KnxTransportState::Disabled) {
    return;
  }

  const uint32_t now = millis();
  if (config_.mode == KnxTransportMode::Routing) {
    const bool wifiConnected = WiFi.status() == WL_CONNECTED;
    const uint32_t localIp = wifiConnected ? static_cast<uint32_t>(WiFi.localIP()) : 0;
    const MulticastNetworkAction action =
        multicastLifecycle_.observe(wifiConnected, localIp, now);
    if (action == MulticastNetworkAction::NetworkLost) {
      synchronizationEpoch_++;
      statistics_.wifiLosses++;
      if (udpOpen_) udp_.stop();
      udpOpen_ = false;
      setState(KnxTransportState::WaitingForWifi,
               "Wi-Fi/IP lost; multicast membership invalidated");
      Serial.printf("[KNX][NETWORK] event=lost wifi_losses=%lu\n",
                    statistics_.wifiLosses);
      return;
    }
    if (action == MulticastNetworkAction::NetworkChanged) {
      synchronizationEpoch_++;
      if (udpOpen_) udp_.stop();
      udpOpen_ = false;
      statistics_.networkGenerations++;
      setState(KnxTransportState::Disconnected,
               "network identity changed; waiting for stable IP");
      Serial.printf("[KNX][NETWORK] event=ip_ready ip=%s generation=%lu stability_ms=%lu\n",
                    WiFi.localIP().toString().c_str(), statistics_.networkGenerations,
                    MulticastLifecycle::kDefaultStabilityMs);
      return;
    }
    if (!wifiConnected || localIp == 0) return;
    if (action == MulticastNetworkAction::JoinReady) {
      startRouting();
      return;
    }
    if (multicastLifecycle_.joinPending()) return;
  }
  if (WiFi.status() != WL_CONNECTED) {
    if (udpOpen_) {
      udp_.stop();
      udpOpen_ = false;
      channelId_ = 0;
    }
    if (state_ != KnxTransportState::WaitingForWifi) {
      setState(KnxTransportState::WaitingForWifi, "station Wi-Fi unavailable");
    }
    return;
  }

  if (state_ == KnxTransportState::WaitingForWifi) {
    setState(KnxTransportState::Disconnected, "station Wi-Fi ready");
  }

  if (udpOpen_) receivePackets();

  if (state_ == KnxTransportState::Disconnected ||
      (state_ == KnxTransportState::Error && retryAllowed_)) {
    if (now - lastConnectAttemptMs_ >= kConnectRetryMs) {
      if (config_.mode == KnxTransportMode::Routing) startRouting();
      else startConnection();
      return;
    }
  } else if (config_.mode == KnxTransportMode::Tunneling &&
             state_ == KnxTransportState::Connecting &&
             now - stateSinceMs_ >= kConnectTimeoutMs) {
    udp_.stop();
    udpOpen_ = false;
    setState(KnxTransportState::Error, "connect timeout");
  } else if (config_.mode == KnxTransportMode::Tunneling &&
             state_ == KnxTransportState::Connected) {
    if (heartbeatPending_ && now - heartbeatSentMs_ >= kHeartbeatTimeoutMs) {
      udp_.stop();
      udpOpen_ = false;
      channelId_ = 0;
      heartbeatPending_ = false;
      setState(KnxTransportState::Error, "heartbeat timeout");
    } else if (!heartbeatPending_ && now - lastHeartbeatMs_ >= kHeartbeatIntervalMs) {
      sendConnectionStateRequest();
    }
  }

  if (config_.mode == KnxTransportMode::Routing &&
      state_ == KnxTransportState::Connected &&
      now - lastStatsMs_ >= kStatsIntervalMs) {
    lastStatsMs_ = now;
    Serial.printf("[KNX][STATS] packets=%lu routing=%lu valid=%lu group=%lu individual=%lu other=%lu invalid=%lu joins=%lu/%lu wifi_losses=%lu network_gen=%lu socket_errors=%lu\n",
                  statistics_.packetsReceived, statistics_.routingIndications,
                  statistics_.validTelegrams, statistics_.groupTelegrams,
                  statistics_.individualTelegrams, statistics_.otherKnxPackets,
                  statistics_.invalidPackets, statistics_.multicastJoinSuccesses,
                  statistics_.multicastJoinAttempts, statistics_.wifiLosses,
                  statistics_.networkGenerations, statistics_.socketErrors);
  }
}

void KNXTransport::disconnect() {
  if (config_.mode == KnxTransportMode::Tunneling &&
      state_ == KnxTransportState::Connected) {
    sendDisconnectRequest();
  }
  udp_.stop();
  udpOpen_ = false;
  channelId_ = 0;
  heartbeatPending_ = false;
  setState(config_.enabled ? KnxTransportState::Disconnected
                           : KnxTransportState::Disabled,
           "local disconnect");
}

KnxTransportState KNXTransport::state() const { return state_; }

const KNXTransport::Statistics& KNXTransport::statistics() const {
  return statistics_;
}

uint32_t KNXTransport::synchronizationEpoch() const {
  return synchronizationEpoch_;
}

const char* KNXTransport::modeName() const {
  return config_.mode == KnxTransportMode::Routing ? "ROUTING" : "TUNNELING";
}

const char* KNXTransport::stateName() const {
  switch (state_) {
    case KnxTransportState::Disabled: return "DISABLED";
    case KnxTransportState::WaitingForWifi: return "WAITING_WIFI";
    case KnxTransportState::Disconnected: return "DISCONNECTED";
    case KnxTransportState::Connecting: return "CONNECTING";
    case KnxTransportState::Connected: return "CONNECTED";
    case KnxTransportState::Error: return "ERROR";
  }
  return "ERROR";
}

void KNXTransport::setKnownDpt1Addresses(const uint16_t* addresses, size_t count) {
  knownDpt1Count_ = min(count, kMaxKnownDpt1);
  for (size_t i = 0; i < knownDpt1Count_; ++i) {
    knownDpt1_[i] = addresses[i];
  }
}

void KNXTransport::setState(KnxTransportState state, const char* reason) {
  state_ = state;
  stateSinceMs_ = millis();
  Serial.printf("[KNX][STATE] state=%s reason=%s\n", stateName(), reason);
}

void KNXTransport::startConnection() {
  udp_.stop();
  udpOpen_ = false;
  if (!udp_.begin(config_.localPort)) {
    lastConnectAttemptMs_ = millis();
    setState(KnxTransportState::Error, "UDP bind failed");
    return;
  }
  udpOpen_ = true;
  retryAllowed_ = true;
  lastConnectAttemptMs_ = millis();
  sendConnectionRequest();
  setState(KnxTransportState::Connecting, "CONNECT_REQUEST sent");
}

void KNXTransport::startRouting() {
  udp_.stop();
  udpOpen_ = false;
  lastConnectAttemptMs_ = millis();
  statistics_.multicastJoinAttempts++;
  if (!udp_.beginMulticast(config_.multicastIp, config_.serverPort)) {
    setState(KnxTransportState::Error, "multicast join failed");
    return;
  }
  udpOpen_ = true;
  statistics_.multicastJoinSuccesses++;
  retryAllowed_ = true;
  lastStatsMs_ = millis();
  setState(KnxTransportState::Connected, "multicast group joined");
  Serial.printf("[KNX][ROUTING] group=%s:%u local_ip=%s join=%lu/%lu outbound_application=%s\n",
                config_.multicastIp.toString().c_str(), config_.serverPort,
                WiFi.localIP().toString().c_str(),
                statistics_.multicastJoinSuccesses,
                statistics_.multicastJoinAttempts,
                config_.applicationTransmitEnabled ? "one-shot-armed" : "blocked");
}

void KNXTransport::sendConnectionRequest() {
  uint8_t data[26]{};
  writeHeader(data, kConnectRequest, sizeof(data));
  writeHpai(data + 6, WiFi.localIP(), config_.localPort);
  writeHpai(data + 14, WiFi.localIP(), config_.localPort);
  data[22] = 0x04;
  data[23] = 0x04;
  data[24] = 0x02;
  data[25] = 0x00;
  sendPacket(data, sizeof(data), config_.serverIp, config_.serverPort);
}

void KNXTransport::sendConnectionStateRequest() {
  uint8_t data[16]{};
  writeHeader(data, kConnectionStateRequest, sizeof(data));
  data[6] = channelId_;
  writeHpai(data + 8, WiFi.localIP(), config_.localPort);
  if (sendPacket(data, sizeof(data), config_.serverIp, config_.serverPort)) {
    heartbeatPending_ = true;
    heartbeatSentMs_ = millis();
  }
}

void KNXTransport::sendDisconnectRequest() {
  uint8_t data[16]{};
  writeHeader(data, kDisconnectRequest, sizeof(data));
  data[6] = channelId_;
  writeHpai(data + 8, WiFi.localIP(), config_.localPort);
  sendPacket(data, sizeof(data), config_.serverIp, config_.serverPort);
}

void KNXTransport::sendTunnelingAck(uint8_t channel, uint8_t sequence,
                                    IPAddress ip, uint16_t port) {
  uint8_t data[10]{};
  writeHeader(data, kTunnelingAck, sizeof(data));
  data[6] = 0x04;
  data[7] = channel;
  data[8] = sequence;
  data[9] = 0x00;
  sendPacket(data, sizeof(data), ip, port);
}

bool KNXTransport::sendPacket(const uint8_t* data, size_t length,
                              IPAddress ip, uint16_t port) {
  const uint16_t service = static_cast<uint16_t>(data[2]) << 8 | data[3];
  if (service != kConnectRequest && service != kConnectionStateRequest &&
      service != kDisconnectRequest && service != kTunnelingAck &&
      !(service == kRoutingIndication && config_.mode == KnxTransportMode::Routing &&
        config_.applicationTransmitEnabled && !transmissionConsumed_)) {
    Serial.printf("[KNX][SECURITY] blocked outbound service=0x%04X\n", service);
    return false;
  }
  if (!udp_.beginPacket(ip, port)) {
    Serial.println("[KNX][ERROR] UDP beginPacket failed");
    return false;
  }
  udp_.write(data, length);
  if (!udp_.endPacket()) {
    Serial.println("[KNX][ERROR] UDP endPacket failed");
    return false;
  }
  return true;
}

bool KNXTransport::transmitAuthorized(uint16_t groupAddress, KnxApci apci) {
  if (config_.mode != KnxTransportMode::Routing ||
      !config_.applicationTransmitEnabled || transmissionConsumed_) {
    Serial.println("[KNX][SECURITY] application transmit blocked: not armed or already consumed");
    return false;
  }
  if (config_.individualAddress == 0 ||
      (config_.individualAddress & 0x00FF) == 0 ||
      groupAddress == 0 || groupAddress != config_.authorizedGroupAddress) {
    Serial.println("[KNX][SECURITY] application transmit blocked: address not authorized");
    return false;
  }
  if ((apci == KnxApci::GroupValueWrite && !config_.groupValueWriteAuthorized) ||
      (apci == KnxApci::GroupValueRead && !config_.groupValueReadAuthorized)) {
    Serial.println("[KNX][SECURITY] application transmit blocked: APCI not authorized");
    return false;
  }
  return true;
}

bool KNXTransport::sendRoutingGroup(uint16_t groupAddress, KnxApci apci, bool value) {
  if (!transmitAuthorized(groupAddress, apci)) return false;
  uint8_t data[kRoutingDpt1PacketSize]{};
  const size_t length = buildRoutingIndication(data, sizeof(data),
                                                config_.individualAddress,
                                                groupAddress, apci, value);
  if (length == 0) {
    Serial.println("[KNX][ERROR] outbound cEMI encoding failed");
    return false;
  }
  if (!sendPacket(data, length, config_.multicastIp, config_.serverPort)) return false;
  transmissionConsumed_ = true;
  char source[16]{};
  char destination[16]{};
  formatIndividualAddress(config_.individualAddress, source, sizeof(source));
  formatGroupAddress(groupAddress, destination, sizeof(destination));
  Serial.printf("[KNX][TX] source=%s destination=%s apci=%s",
                source, destination, apciName(apci));
  if (apci == KnxApci::GroupValueWrite) {
    Serial.printf(" dpt=1.001 value=%s", value ? "ON" : "OFF");
  }
  Serial.println(" one_shot=consumed");
  return true;
}

bool KNXTransport::sendGroupValueWriteDpt1(uint16_t groupAddress, bool value) {
  return sendRoutingGroup(groupAddress, KnxApci::GroupValueWrite, value);
}

bool KNXTransport::sendGroupValueRead(uint16_t groupAddress) {
  return sendRoutingGroup(groupAddress, KnxApci::GroupValueRead, false);
}

void KNXTransport::receivePackets() {
  while (true) {
    const int packetLength = udp_.parsePacket();
    if (packetLength == 0) break;
    if (packetLength < 0) {
      synchronizationEpoch_++;
      statistics_.socketErrors++;
      udp_.stop();
      udpOpen_ = false;
      lastConnectAttemptMs_ = millis();
      setState(KnxTransportState::Error,
               "UDP parse error; multicast rejoin scheduled");
      Serial.printf("[KNX][NETWORK] event=socket_error count=%lu\n",
                    statistics_.socketErrors);
      break;
    }
    statistics_.packetsReceived++;
    if (packetLength > static_cast<int>(kMaxPacketSize)) {
      Serial.printf("[KNX][ERROR] oversized packet=%d\n", packetLength);
      statistics_.invalidPackets++;
      while (udp_.available()) udp_.read();
      continue;
    }
    const int received = udp_.read(packet_, packetLength);
    if (received > 0) {
      handlePacket(packet_, received, udp_.remoteIP(), udp_.remotePort());
      if (!udpOpen_) return;
    }
  }
}

void KNXTransport::handlePacket(const uint8_t* data, size_t length,
                                IPAddress remoteIp, uint16_t remotePort) {
  if (length < 6 || data[0] != 0x06 || data[1] != 0x10) {
    Serial.println("[KNX][ERROR] invalid KNXnet/IP header");
    statistics_.invalidPackets++;
    return;
  }
  const uint16_t service = static_cast<uint16_t>(data[2]) << 8 | data[3];
  const uint16_t declaredLength = static_cast<uint16_t>(data[4]) << 8 | data[5];
  if (declaredLength != length) {
    Serial.printf("[KNX][ERROR] length declared=%u received=%u\n",
                  declaredLength, static_cast<unsigned>(length));
    statistics_.invalidPackets++;
    return;
  }

  if (config_.mode == KnxTransportMode::Routing) {
    if (service != kRoutingIndication) {
      statistics_.otherKnxPackets++;
      if (ignoredServiceLogs_ < 10) {
        ignoredServiceLogs_++;
        Serial.printf("[KNX][IGNORED] service=0x%04X source=%s:%u\n", service,
                      remoteIp.toString().c_str(), remotePort);
      }
      return;
    }
    statistics_.routingIndications++;
    if (length < 16) {
      Serial.printf("[KNX][ERROR] short ROUTING_INDICATION length=%u\n",
                    static_cast<unsigned>(length));
      statistics_.invalidPackets++;
      return;
    }
    handleCemi(data + 6, length - 6);
    return;
  }

  if (service == kConnectResponse && state_ == KnxTransportState::Connecting && length >= 8) {
    if (data[7] == 0x00) {
      channelId_ = data[6];
      expectedSequence_ = 0;
      lastHeartbeatMs_ = millis();
      heartbeatPending_ = false;
      setState(KnxTransportState::Connected, "CONNECT_RESPONSE accepted");
      Serial.printf("[KNX] channel=%u server=%s:%u\n", channelId_,
                    config_.serverIp.toString().c_str(), config_.serverPort);
    } else {
      Serial.printf("[KNX][ERROR] CONNECT_RESPONSE status=0x%02X (%s)\n",
                    data[7], connectStatusName(data[7]));
      udp_.stop();
      udpOpen_ = false;
      retryAllowed_ = false;
      setState(KnxTransportState::Error,
               "connection rejected; automatic retry paused");
    }
  } else if (service == kConnectionStateResponse && length >= 8) {
    heartbeatPending_ = false;
    if (data[6] == channelId_ && data[7] == 0x00) {
      lastHeartbeatMs_ = millis();
      Serial.println("[KNX][HEARTBEAT] alive");
    } else {
      udp_.stop();
      udpOpen_ = false;
      channelId_ = 0;
      setState(KnxTransportState::Error, "heartbeat rejected");
    }
  } else if (service == kTunnelingRequest && length >= 11) {
    const uint8_t structureLength = data[6];
    if (structureLength != 0x04) {
      Serial.println("[KNX][ERROR] invalid tunneling header");
      return;
    }
    const uint8_t channel = data[7];
    const uint8_t sequence = data[8];
    sendTunnelingAck(channel, sequence, remoteIp, remotePort);
    if (state_ != KnxTransportState::Connected || channel != channelId_) {
      Serial.printf("[KNX][ERROR] telegram for invalid channel=%u\n", channel);
      return;
    }
    if (sequence != expectedSequence_) {
      Serial.printf("[KNX][ERROR] sequence expected=%u received=%u\n",
                    expectedSequence_, sequence);
      return;
    }
    expectedSequence_++;
    handleCemi(data + 10, length - 10);
  } else if (service == kDisconnectResponse) {
    udp_.stop();
    udpOpen_ = false;
    channelId_ = 0;
    setState(KnxTransportState::Disconnected, "DISCONNECT_RESPONSE received");
  }
}

void KNXTransport::handleCemi(const uint8_t* data, size_t length) {
  KnxTelegram telegram;
  if (!decodeCemi(data, length, telegram)) {
    Serial.println("[KNX][ERROR] unsupported or malformed cEMI");
    statistics_.invalidPackets++;
    return;
  }
  statistics_.validTelegrams++;
  if (telegram.groupDestination) statistics_.groupTelegrams++;
  else statistics_.individualTelegrams++;
  char source[16]{};
  char destination[16]{};
  formatIndividualAddress(telegram.source, source, sizeof(source));
  if (telegram.groupDestination) {
    formatGroupAddress(telegram.destination, destination, sizeof(destination));
  } else {
    formatIndividualAddress(telegram.destination, destination, sizeof(destination));
  }
  Serial.printf("[KNX][RX] source=%s destination=%s address_type=%s apci=%s",
                source, destination, telegram.groupDestination ? "group" : "individual",
                apciName(telegram.apci));
  if (telegram.groupDestination && isKnownDpt1(telegram.destination) &&
      telegram.hasInlineValue &&
      (telegram.apci == KnxApci::GroupValueResponse ||
       telegram.apci == KnxApci::GroupValueWrite)) {
    Serial.printf(" dpt=1.001 value=%s", telegram.inlineValue ? "ON" : "OFF");
  } else if (telegram.hasInlineValue) {
    Serial.printf(" value=raw:%u dpt=unknown", telegram.inlineValue ? 1 : 0);
  }
  Serial.println();
  if (handler_) handler_(telegram);
}

bool KNXTransport::isKnownDpt1(uint16_t groupAddress) const {
  for (size_t i = 0; i < knownDpt1Count_; ++i) {
    if (knownDpt1_[i] == groupAddress) return true;
  }
  return false;
}

bool KNXTransport::decodeCemi(const uint8_t* data, size_t length, KnxTelegram& telegram) {
  if (!data || length < 10 || (data[0] != 0x29 && data[0] != 0x2E)) return false;
  const size_t base = 2 + data[1];
  if (base + 9 > length) return false;
  const uint8_t control2 = data[base + 1];
  telegram.source = static_cast<uint16_t>(data[base + 2]) << 8 | data[base + 3];
  telegram.destination = static_cast<uint16_t>(data[base + 4]) << 8 | data[base + 5];
  telegram.groupDestination = (control2 & 0x80) != 0;
  const uint8_t npduLength = data[base + 6];
  if (base + 8 + npduLength > length || npduLength < 1) return false;
  const uint8_t* tpdu = data + base + 7;
  const uint8_t apci = static_cast<uint8_t>(((tpdu[0] & 0x03) << 2) |
                                            ((tpdu[1] >> 6) & 0x03));
  telegram.apci = apci <= 2 ? static_cast<KnxApci>(apci) : KnxApci::Other;
  telegram.hasInlineValue = npduLength == 1;
  telegram.inlineValue = (tpdu[1] & 0x01) != 0;
  return true;
}

size_t KNXTransport::encodeRoutingGroupCemi(uint8_t* output, size_t capacity,
                                            uint16_t source, uint16_t groupAddress,
                                            KnxApci apci, bool value) {
  return KnxCemiCodec::encodeRoutingGroup(output, capacity, source, groupAddress,
                                          apci, value);
}

size_t KNXTransport::encodeTunnelingGroupCemi(uint8_t* output, size_t capacity,
                                              uint16_t source, uint16_t groupAddress,
                                              KnxApci apci, bool value) {
  return KnxCemiCodec::encodeTunnelingGroup(output, capacity, source, groupAddress,
                                            apci, value);
}

size_t KNXTransport::buildRoutingIndication(uint8_t* output, size_t capacity,
                                            uint16_t source, uint16_t groupAddress,
                                            KnxApci apci, bool value) {
  return KnxCemiCodec::buildRoutingIndication(output, capacity, source,
                                               groupAddress, apci, value);
}

void KNXTransport::formatIndividualAddress(uint16_t address, char* output, size_t length) {
  snprintf(output, length, "%u.%u.%u", (address >> 12) & 0x0F,
           (address >> 8) & 0x0F, address & 0xFF);
}

void KNXTransport::formatGroupAddress(uint16_t address, char* output, size_t length) {
  snprintf(output, length, "%u/%u/%u", (address >> 11) & 0x1F,
           (address >> 8) & 0x07, address & 0xFF);
}

bool KNXTransport::runDecoderSelfTest() {
  const uint8_t cemi[] = {
      0x29, 0x00, 0xBC, 0xE0, 0x11, 0x0A, 0x0A, 0x03, 0x01, 0x00, 0x81,
  };
  KnxTelegram telegram;
  if (!decodeCemi(cemi, sizeof(cemi), telegram)) return false;
  return telegram.source == 0x110A && telegram.destination == 0x0A03 &&
         telegram.groupDestination && telegram.apci == KnxApci::GroupValueWrite &&
         telegram.hasInlineValue && telegram.inlineValue;
}

bool KNXTransport::runEncoderSelfTest() {
  const uint8_t expectedWriteOn[] = {
      0x06, 0x10, 0x05, 0x30, 0x00, 0x11,
      0x29, 0x00, 0xBC, 0xE0, 0x11, 0xFE, 0x0A, 0x03, 0x01, 0x00, 0x81,
  };
  uint8_t packet[kRoutingDpt1PacketSize]{};
  size_t length = buildRoutingIndication(packet, sizeof(packet), 0x11FE, 0x0A03,
                                         KnxApci::GroupValueWrite, true);
  if (length != sizeof(expectedWriteOn) ||
      memcmp(packet, expectedWriteOn, sizeof(expectedWriteOn)) != 0) return false;

  uint8_t tunneling[KnxCemiCodec::kGroupCemiSize]{};
  length = encodeTunnelingGroupCemi(tunneling, sizeof(tunneling), 0x11FE,
                                    0x0A03, KnxApci::GroupValueWrite, true);
  if (length != sizeof(tunneling) || tunneling[0] != 0x11) return false;
  for (size_t i = 1; i < sizeof(tunneling); ++i) {
    if (tunneling[i] != expectedWriteOn[i + 6]) return false;
  }

  length = buildRoutingIndication(packet, sizeof(packet), 0x11FE, 0x0A03,
                                  KnxApci::GroupValueWrite, false);
  if (length != kRoutingDpt1PacketSize || packet[16] != 0x80) return false;

  length = buildRoutingIndication(packet, sizeof(packet), 0x11FE, 0x0A03,
                                  KnxApci::GroupValueRead, false);
  if (length != kRoutingDpt1PacketSize || packet[16] != 0x00) return false;

  if (buildRoutingIndication(packet, sizeof(packet), 0x1100, 0x0A03,
                             KnxApci::GroupValueWrite, false) != 0) return false;
  if (buildRoutingIndication(packet, sizeof(packet), 0x11FE, 0,
                             KnxApci::GroupValueWrite, false) != 0) return false;
  if (buildRoutingIndication(packet, 16, 0x11FE, 0x0A03,
                             KnxApci::GroupValueWrite, false) != 0) return false;
  return true;
}

