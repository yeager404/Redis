#pragma once

#include "redis/protocol/resp/RespValue.h"
#include <string>

namespace redis::protocol::resp {
  class RespEncoder {
    public: 
      // convert one value into RESP2 wire bytes.
      std::string encode(const RespValue& value) const;
  };
} // namespace redis::protocol
