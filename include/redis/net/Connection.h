#pragma once

#include "IoEvent.h"
#include "redis/net/Buffer.h"
#include "redis/net/ISocket.h"
#include "redis/protocol/ProtocolCodec.h"
#include "redis/application/CommandDispatcher.h"

#include <memory>

namespace redis::net {
  class Connection {
    public:

      Connection(ConnectionId id, std::unique_ptr<ISocket> socket, std::unique_ptr<protocol::ProtocolCodec> codec);
      ConnectionId id() const noexcept;

      // Read available bytes, decode requests, and queue responses.
      bool onReadable(application::CommandDispatcher& dispatcher);
      bool onWritable();
      bool wantsWrite() const noexcept;

    private:
      ConnectionId id_;

      std::unique_ptr<ISocket> socket_;
      std::unique_ptr<protocol::ProtocolCodec> codec_;

      Buffer input_;
      Buffer output_;
      bool peerClosed_ = false;
      bool closed_ = false;
  };
} // namespace redis::net
