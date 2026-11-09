#pragma once

#include "redis/net/IoEvent.h"

#include <chrono>
#include <ratio>
#include <vector>

namespace redis::net {
  enum class BackendModel {
    Readiness,
    Completion
  };

  class IEventBackend {
    public:
      virtual ~IEventBackend() = default;

      virtual BackendModel model() const noexcept = 0;

      // Wait until events arrive or the timeout expires.
      virtual std::vector<IoEvent> wait(std::chrono::milliseconds timeout) = 0;

      // Wake the eventloop during the shutdown or when work is queued.
      virtual void wakeup() = 0;
  };
} // namespace redis::net
