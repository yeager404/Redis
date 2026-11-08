#pragma once

#include "redis/protocol/ProtocolCodec.h"
#include "redis/protocol/resp/RespEncoder.h"
#include "redis/protocol/resp/RespParser.h"

#include <string_view>

namespace redis::protocol::resp {
  class Resp2Codec final : public protocol::ProtocolCodec {
    public:
      explicit Resp2Codec(ParseLimits limits = {});
      protocol::DecodeResult decodeOne(std::string_view input ) override;
      std::string encode(const application::Response& response) override;

    private:
      RespParser parser_;
      RespEncoder encoder_;
  };
} // namespace redis::protocol::resp
