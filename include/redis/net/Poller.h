#pragma once

#include <cstdint>
#include <memory>
#include <vector>

namespace redis::net {
  struct PollEvent {
    int fd;
    std::uint32_t events;
  };

  class Poller {
    public:
      virtual ~Poller() = default;
      virtual void add(int fd, std::uint32_t events) = 0;
      virtual void modify(int fd, std::uint32_t events) = 0;
      virtual void remove(int fd) = 0;

      virtual int wait( std::vector<PollEvent>& events, int timeoutMs) = 0;
  };

  std::unique_ptr<Poller> createPoller();
}

// namespace redis::net

