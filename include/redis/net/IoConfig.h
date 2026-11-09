#pragma once

#include <cstdint>
#include <string>

namespace redis::net {
  struct IoConfig {
    std::string host = "127.0.0.1";
    std::uint16_t port = 6379;
    int backlog = 128;
  };
} // namespace redis::net
