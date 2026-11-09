#pragma once
#include "IEventBackend.h"
#include "ISocket.h"
#include "ITcpListener.h"
#include "IoEvent.h"
#include "redis/net/IEventBackend.h"

namespace redis::net {
  class ISocket;
  class ITcpListener;

  class IReadinessBackend : public IEventBackend {
    public:
      virtual void addListener(ConnectionId id, ITcpListener& listener) = 0;

      virtual void add (ConnectionId id, ISocket& socket, bool readEnabled, bool writeEnabled) = 0;
      
      virtual void modify(ConnectionId id, bool readEnabled, bool writeEnabled) = 0;

      virtual void remove(ConnectionId id) = 0;
  };
} // namespace redis::net
