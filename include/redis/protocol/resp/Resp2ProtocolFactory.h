#pragma once

#include "redis/protocol/ProtocolFactory.h"
#include "redis/protocol/resp/RespParser.h"

namespace redis::protocol::resp {
  class Resp2ProtocolFactory final : public protocol::ProtocolFactory{
    public:
      explicit Resp2ProtocolFactory(ParseLimits limits = {});

      std::unique_ptr<protocol::ProtocolCodec> createCodec() const override;
      
    private:
      ParseLimits limits_;
  };
} // namespace redis::protocol::resp
