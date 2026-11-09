#pragma once

#include "redis/net/IoBackendFactory.h"
#include <memory>

namespace redis::net::linux_backend {
  class LinuxIoBackendFactory final : public IoBackendFactory{
    public:
      std::unique_ptr<IEventBackend> createBackend(const IoConfig& config) override;
      std::unique_ptr<ITcpListener> createListener(const IoConfig& config) override;
  };
} // namespace redis::net::linux_backend
