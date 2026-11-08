#pragma once

#include "redis/application/Request.h"
#include "redis/application/Response.h"

#include <cstddef>
#include <iterator>
#include <string>
#include <string_view>

namespace redis::protocol {
  enum class DecodeStatus {
    Complete,
    Incomplete,
    Error
  };

  struct DecodeResult {
    DecodeStatus status = DecodeStatus::Incomplete;

    // Bytes belonging to the complete request.
    std::size_t consumed = 0;
    application::Request request;
    std::string error;
  };

  class ProtocolCodec {
    public :
      virtual ~ProtocolCodec() = default;

      // Parse one request without modifying the input buffer
      virtual DecodeResult decodeOne(std::string_view input) = 0;

      // Encode an application response into wire bytes.
      virtual std::string encode(const application::Response& response) = 0;
  };
} // namespace redis::protocol
