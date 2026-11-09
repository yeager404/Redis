#pragma once

#include "IoConfig.h"
#include "redis/net/IEventBackend.h"
#include "redis/net/ITcpListener.h"
#include "redis/net/IoConfig.h"

#include <memory>

namespace redis::net {
  class IoBackendFactory {
    public:
      virtual ~IoBackendFactory() = default;
      virtual std::unique_ptr<IEventBackend> createBackend(const IoConfig& config) = 0;
      virtual std::unique_ptr<ITcpListener> createListener(const IoConfig& config) = 0;
  };
} // namespace redis::net
