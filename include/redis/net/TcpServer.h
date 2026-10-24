#pragma once

namespace redis::net{
  class TcpServer {
    public:
      explicit TcpServer(int port);
      ~TcpServer();

      TcpServer(const TcpServer&) = delete;
      TcpServer& operator = (const TcpServer&) = delete;

      int fd() const;

      int acceptClient();

    private:
      int fd_ = -1;
  };
}

// namespace redis::net
