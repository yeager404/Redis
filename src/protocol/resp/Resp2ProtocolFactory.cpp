#include "redis/protocol/resp/Resp2ProtocolFactory.h"
#include "redis/protocol/resp/Resp2Codec.h"

#include <memory>

namespace redis::protocol::resp {
  Resp2ProtocolFactory::Resp2ProtocolFactory(ParseLimits limits): limits_(limits){}

  std::unique_ptr<protocol::ProtocolCodec>Resp2ProtocolFactory::createCodec() const{
    return std::make_unique<Resp2Codec>(limits_);
  }
} // namespace redis::protocol::resp
