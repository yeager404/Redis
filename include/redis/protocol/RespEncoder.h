#pragma once

#include "redis/protocol/RespValue.h"

#include <string>

namespace redis::protocol {
  class RespEncoder {
    public: 
      std::string encode(const RespValue& value) const;
  };
} // namespace redis::protocol
