#pragma once
#include <cstddef>

namespace redis::net {
  enum class IoStatus {
    Progress,
    WouldBlock,
    PeerClosed,
    Error
  };

  struct IoResult {
    IoStatus status = IoStatus::Error;
    std::size_t bytesTransferred = 0;

    // Native error code, when applicable.
    // Interpret it using the backend that produced it
    int errorCode = 0;
  };
} // namespace redis::net
