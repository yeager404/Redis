#pragma once
#include "PosixSocket.h"
#include "redis/net/ITcpListener.h"
#include "redis/net/IoConfig.h"

namespace redis::net::linux_backend {
  class PosixTcpListener final : public ITcpListener{
    public:
      explicit PosixTcpListener(const IoConfig& config);
      ~PosixTcpListener() override;

      PosixTcpListener(const PosixTcpListener&) = delete;
      PosixTcpListener& operator = (const PosixTcpListener&) = delete;
      
      int nativeHandle() const noexcept;

      AcceptResult accept() override;
      void close() noexcept override;

    private:
      int fd_ = -1;
  };
} // namespace redis::net:linux_backend
