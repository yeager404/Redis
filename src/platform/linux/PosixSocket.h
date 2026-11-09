#pragma once

#include "redis/net/ISocket.h"
#include <cstddef>

namespace redis::net::linux_backend {
  class PosixSocket final : public ISocket {
    public:
      explicit PosixSocket(int fd);
      ~PosixSocket() override;

      PosixSocket(const PosixSocket&) = delete;
      PosixSocket& operator = (const PosixSocket&) = delete;

      int nativeHandler() const noexcept;
      IoResult read(std::span<std::byte> buffer) override;

      IoResult write(std::span<const std::byte> buffer) ovverride;

      void close() noexcept override;

    private:
      int fd_ = -1;
  };
} // namespace redis::net::linux_backend
