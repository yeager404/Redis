#pragma once
#include "IEventBackend.h"
#include "IoEvent.h"
#include "redis/net/IEventBackend.h"
#include <memory>

namespace redis::net{
  class IoOperationContext;

  class ICompletionBackend : public IEventBackend {
    virtual void submitConnection(ConnectionId listenerId) = 0;
    virtual void submitRead(ConnectionId connectionId, std::shared_ptr<IoOperationContext>operation) = 0;
    virtual void cancel(ConnectionId connectionId) = 0;
  };
} // namespace redis::net
