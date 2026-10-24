#pragma once

#include "redis/net/ConnectionManager.h"
#include "redis/net/Poller.h"
#include "redis/net/TcpServer.h"

#include <memory>

namespace redis::net {
  class EventLoop {
    public:
      EventLoop(TcpServer& server, std::unique_ptr<Poller> poller);

      void run();
    private:
      void handleAccept();
      void handleClient(int fd, unsigned int events);

      bool readClient(Connection& conn);
      bool writeClient(Connection& conn);

      void closeClient(int fd);
      void updateInterest(Connection& conn);

      TcpServer& server_;
      std::unique_ptr<Poller> poller_;
      ConnectionManager connections_;

      bool running_ = true; 
  };
}// namespace redis::net
