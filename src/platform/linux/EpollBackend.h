#pragma once

#include "redis/net/IReadinessBackend.h"
#include <chrono>
#include <unordered_map>
namespace redis::net::linux_backend {
  class EpollBackend final : public IReadinessBackend {
    public:
      EpollBackend();
      ~EpollBackend() override;

      BackendModel model() const noexcept override;
      std::vector<IoEvent> wait(std::chrono::milliseconds timeout) override;

      void wakeup() override;

      void addListener(ConnectionId id, ITcpListener& listener) override;

      void add(ConnectionId id, ISocket& socket, bool readEnabled, bool writeEnabled) override;

      void modify(ConnectionId id, bool readEnabled, bool write enabled) override;

      void remove(ConnectionId id) override;

    private:
      void registerFd(int fd, ConnectionId id , bool readEnabled, bool writeEnabled, bool isListener);
      int epollFd_ = -1;
      int wakeFd_ = -1;

      // Connection IDs are stored in epoll event data.
      // ID 0 is reserved for the wakeup descriptor.
      std::unordered_map<ConnectionId, id> fds_;
  };
} // namespace redis::net::linux_backend
