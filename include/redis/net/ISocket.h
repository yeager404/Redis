#pragma once

#include "IoResult.h"
#include "redis/net/IoResult.h"

#include <cstddef>
#include <span>

namespace redis::net{
  class ISocket {
    public:
      virtual ~ISocket() = default;

      // Read into the caller's buffer  
      virtual IoResult read(std::span<std::byte> buffer) = 0;
    
      // Write some bytes from the caller's buffer.
      // A successful write may transfer fewer bytes than requested.
      
      virtual IoResult write(std::span<const std::byte> buffer) = 0;
      virtual void close() noexcept = 0;
  };
} // namespace redis::net
