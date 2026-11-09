#pragma once

#include "IoResult.h"
#include "redis/net/ISocket.h"

#include <memory>
#include <string>

namespace redis::net {
  struct AcceptResult {
    std::unique_ptr<ISocket> socket;
    IoResult result;
  };

  class ITcpListener {
    public:
      virtual ~ITcpListener() = default;

      // Readiness-based implementation accept when the listener
      // is ready. Completion-based implementation may deliver an
      // accepted socket through an asynchronous completion instead.

      virtual AcceptResult accept() = 0;
      virtual void close() noexcept = 0;
  };
}
