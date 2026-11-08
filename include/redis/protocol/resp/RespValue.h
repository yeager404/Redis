#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <variant>
#include <vector>

namespace redis::protocol {

  struct SimpleString {
    std::string value;
  };

  struct Error {
    std::string value;
  };

  struct Integer{
    std::int64_t value;
  };

  struct BulkString {
    // nullopt represents a RESP2 null bulk string.
    std::optional<std::string> value;
  };

  struct RespValue {
  
    using Array = std::optional<std::vector<RespValue>>;

    //monostate is used as a temporary/default state.
    // Array = nullopt represents a null array: *-1\r\n
    using Data = std::variant<
      std::monostate,
      SimpleString,
      Error,
      Integer,
      BulkString,
      Array 
    >;

    Data data;
  };
} // namespace redis::protocol
