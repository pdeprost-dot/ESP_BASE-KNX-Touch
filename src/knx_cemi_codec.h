#pragma once

#include <cstddef>
#include <cstdint>

enum class KnxApci : uint8_t {
  GroupValueRead = 0,
  GroupValueResponse = 1,
  GroupValueWrite = 2,
  Other = 0xFF,
};

namespace KnxCemiCodec {
constexpr uint8_t kLDataRequest = 0x11;
constexpr uint8_t kLDataIndication = 0x29;
constexpr size_t kGroupCemiSize = 11;
constexpr size_t kRoutingPacketSize = 17;

constexpr size_t encodeGroup(uint8_t* output, size_t capacity,
                             uint8_t messageCode, uint16_t source,
                             uint16_t groupAddress, KnxApci apci, bool value) {
  if (!output || capacity < kGroupCemiSize ||
      (messageCode != kLDataRequest && messageCode != kLDataIndication) ||
      source == 0 || (source & 0x00FF) == 0 || groupAddress == 0 ||
      (apci != KnxApci::GroupValueRead && apci != KnxApci::GroupValueWrite)) {
    return 0;
  }
  output[0] = messageCode;
  output[1] = 0x00;  // No additional information.
  output[2] = 0xBC;  // Standard frame, low priority, no repeat-on-error.
  output[3] = 0xE0;  // Group destination, hop count 6, standard format.
  output[4] = source >> 8;
  output[5] = source & 0xFF;
  output[6] = groupAddress >> 8;
  output[7] = groupAddress & 0xFF;
  output[8] = 0x01;  // Two APDU octets, encoded as NPDU length minus one.
  output[9] = 0x00;  // Unnumbered data; APCI high bits are zero here.
  output[10] = apci == KnxApci::GroupValueWrite
                   ? static_cast<uint8_t>(0x80 | (value ? 0x01 : 0x00))
                   : 0x00;
  return kGroupCemiSize;
}

constexpr size_t encodeRoutingGroup(uint8_t* output, size_t capacity,
                                    uint16_t source, uint16_t groupAddress,
                                    KnxApci apci, bool value) {
  // A ROUTING_INDICATION carries the link-layer indication primitive.
  return encodeGroup(output, capacity, kLDataIndication, source, groupAddress,
                     apci, value);
}

constexpr size_t encodeTunnelingGroup(uint8_t* output, size_t capacity,
                                      uint16_t source, uint16_t groupAddress,
                                      KnxApci apci, bool value) {
  // A TUNNELING_REQUEST submits a link-layer request to the tunnel endpoint.
  return encodeGroup(output, capacity, kLDataRequest, source, groupAddress,
                     apci, value);
}

constexpr size_t buildRoutingIndication(uint8_t* output, size_t capacity,
                                        uint16_t source, uint16_t groupAddress,
                                        KnxApci apci, bool value) {
  if (!output || capacity < kRoutingPacketSize) return 0;
  const size_t cemiLength = encodeRoutingGroup(output + 6, capacity - 6, source,
                                               groupAddress, apci, value);
  if (cemiLength == 0) return 0;
  output[0] = 0x06;
  output[1] = 0x10;
  output[2] = 0x05;
  output[3] = 0x30;
  output[4] = 0x00;
  output[5] = static_cast<uint8_t>(6 + cemiLength);
  return 6 + cemiLength;
}
}  // namespace KnxCemiCodec
