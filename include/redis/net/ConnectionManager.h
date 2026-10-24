#pragma once

#include "redis/net/Connection.h"

#include <memory.h>
#include <memory>
#include <unordered_map>

namespace redis::net {
  class ConnectionManager {
    public:
      Connection& add(int fd);
      Connection* get(int fd);
      void remove(int fd);

    private:
      std::unordered_map<int,std::unique_ptr<Connection>> connections_;
  };
}
// namespace redis::net
