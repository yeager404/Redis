#pragma once

#include "redis/protocol/ProtocolCodec.h"
#include <memory>

namespace redis::protocol {
  class ProtocolFactory {
    public:
      virtual ~ProtocolFactory() = default;
      virtual std::unique_ptr<ProtocolCodec> createCodec() const = 0;
  };
} // namespace redis::protocol
