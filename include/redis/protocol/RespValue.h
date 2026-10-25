#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <variant>
#include <vector>

namespace redis::protocol {
  struct RespValue;

  struct RespSimpleString {
    std::string value;
  };

  struct RespError {
    std::string value;
  };

  struct RespInteger{
    std::int64_t value;
  };

  struct RespBulkString {
    // nullopt represents a RESP2 null bulk string.
    std::optional<std::string> value;
  };

  struct RespArray {
    // nullopt represents a RESP2 null array.
    std::optional<std::vector<RespValue>> values;
  };

  struct RespValue {
    using Data = std::variant<
      RespSimpleString,
      RespError,
      RespInteger,
      RespBulkString,
      RespArray 
    >;

    Data data;
  };
} // namespace redis::protocol
