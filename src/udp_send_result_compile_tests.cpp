#include "udp_send_result.h"

static_assert(udpSendSucceeded(true, 17, 17, true),
              "A complete UDP send must succeed");
static_assert(!udpSendSucceeded(false, 0, 17, false),
              "beginPacket failure must fail");
static_assert(!udpSendSucceeded(true, 16, 17, false),
              "Short UDP write must fail");
static_assert(!udpSendSucceeded(true, 17, 17, false),
              "endPacket failure must fail");
