#pragma once

#include <stddef.h>

constexpr bool udpSendSucceeded(bool beginOk, size_t written,
                                size_t expected, bool endOk) {
  return beginOk && written == expected && endOk;
}
