#pragma once

#include "IoResult.h"
#include <cstddef>
#include <cstdint>

namespace redis::net {
  using ConnectionId = std::uint64_t;
  using OperationId = std::uint64_t;


  enum class IoEventKind {
    ReadReady,
    WriteReady,
    PeerClosed,
    OperationCompleted,
    Error
  };
  

  enum class IoOperation {
    Accept, 
    Read, 
    Write
  };

  struct IoEvent {
    IoEventKind kind = IoEventKind::Error;
    ConnectionId connectionId = 0;
    OperationId operationId = 0;
    IoOperation operation = IoOperation::Read;

    std::size_t bytesTransferrred = 0;
    int errorCode = 0;
  };
} // namespace redis::net
