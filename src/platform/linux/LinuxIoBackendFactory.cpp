#include "LinuxIoBackendFactory.h"

#include "EpollBackend.h"
#include "PosixSocket.h"

#include <memory>

namespace redis::net::linux_backend {
  std::unique_ptr<IEventBackend>LinuxIoBackendFactory::createBackend(const IoConfig& config){
    (void) config;
    return std::make_unique<EpollBackend>();
  }

  std::unique_ptr<ITcpListener>LinuxIoBackendFactory::createListener(const IoConfig& config){
    return std::make_unique<PosixTcpListener>(config);
  }
} // namespace redis::net::linux_backend
