#pragma once

#include "redis/protocol/RespValue.h"

#include <cstddef>
#include <iterator>
#include <string>
#include <string_view>

namespace redis::protocol {
  enum class ParseStatus {
    Complete,
    Incomplete,
    Error 
  };

  struct ParseLimits {
    std::size_t maxBulkStringBytes = 64*1024*1024;
    std::size_t maxArrayElements = 100000;
    std::size_t maxNestingDepth = 64;
  };

  struct RespParseResult {
    ParseStatus status = ParseStatus::Incomplete;
    std::size_t consumed = 0;
    RespValue value{RespSimpleString{""}};
    std::string error;
  };

  class RespParser{
    public:
      explicit RespParser(ParseLimits limits = {});

      // parseOne takes string_view instead of a std::string.
      // This is a zero copy optimization. It looks directly at the memory sitting inside
      // your memory Buffer.
      RespParseResult parseOne(std::string_view input) const;

    private:
      ParseLimits limits_;
  };
} // namespace redis::protocol

