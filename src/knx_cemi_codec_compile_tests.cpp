#include "knx_cemi_codec.h"

#include <array>

namespace {
constexpr bool routingWriteOnIsExact() {
  std::array<uint8_t, KnxCemiCodec::kRoutingPacketSize> packet{};
  constexpr std::array<uint8_t, KnxCemiCodec::kRoutingPacketSize> expected{
      0x06, 0x10, 0x05, 0x30, 0x00, 0x11,
      0x29, 0x00, 0xBC, 0xE0, 0x11, 0xFA, 0x10, 0x00, 0x01, 0x00, 0x81,
  };
  if (KnxCemiCodec::buildRoutingIndication(
          packet.data(), packet.size(), 0x11FA, 0x1000,
          KnxApci::GroupValueWrite, true) != packet.size()) return false;
  for (size_t i = 0; i < packet.size(); ++i) {
    if (packet[i] != expected[i]) return false;
  }
  return true;
}

constexpr bool transportsRemainDistinct() {
  std::array<uint8_t, KnxCemiCodec::kGroupCemiSize> routing{};
  std::array<uint8_t, KnxCemiCodec::kGroupCemiSize> tunneling{};
  if (KnxCemiCodec::encodeRoutingGroup(
          routing.data(), routing.size(), 0x11FA, 0x1000,
          KnxApci::GroupValueWrite, true) != routing.size()) return false;
  if (KnxCemiCodec::encodeTunnelingGroup(
          tunneling.data(), tunneling.size(), 0x11FA, 0x1000,
          KnxApci::GroupValueWrite, true) != tunneling.size()) return false;
  if (routing[0] != 0x29 || tunneling[0] != 0x11) return false;
  for (size_t i = 1; i < routing.size(); ++i) {
    if (routing[i] != tunneling[i]) return false;
  }
  return true;
}

constexpr bool dpt1AndErrorsAreChecked() {
  std::array<uint8_t, KnxCemiCodec::kRoutingPacketSize> packet{};
  if (KnxCemiCodec::buildRoutingIndication(
          packet.data(), packet.size(), 0x11FA, 0x1000,
          KnxApci::GroupValueWrite, false) != packet.size() ||
      packet[16] != 0x80) return false;
  if (KnxCemiCodec::buildRoutingIndication(
          packet.data(), packet.size(), 0x11FA, 0x1000,
          KnxApci::GroupValueRead, false) != packet.size() ||
      packet[16] != 0x00) return false;
  return KnxCemiCodec::buildRoutingIndication(
             packet.data(), packet.size(), 0x1100, 0x1000,
             KnxApci::GroupValueWrite, true) == 0 &&
         KnxCemiCodec::buildRoutingIndication(
             packet.data(), packet.size(), 0x11FA, 0,
             KnxApci::GroupValueWrite, true) == 0 &&
         KnxCemiCodec::buildRoutingIndication(
             packet.data(), packet.size() - 1, 0x11FA, 0x1000,
             KnxApci::GroupValueWrite, true) == 0;
}

static_assert(routingWriteOnIsExact(), "Routing frame regression");
static_assert(transportsRemainDistinct(), "Routing/tunneling cEMI regression");
static_assert(dpt1AndErrorsAreChecked(), "DPT1 or validation regression");
}  // namespace
