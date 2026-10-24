#pragma once

#include "redis/net/Buffer.h"

namespace redis::net {
  class Connection {
    public:
      explicit Connection(int fd);
      ~Connection();

      Connection(const Connection&) = delete;
      Connection& operator = (const Connection&) = delete;

      int fd() const;

      Buffer& inputBuffer();
      Buffer& outputBuffer();
      
      bool peerClosed() const;
      void markPeerClosed();

    private:
      int fd_;
      Buffer input_;
      Buffer output_;
      bool peerClosed_ = false;
  };
} // namespace redis::net
