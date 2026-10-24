#include "redis/net/TcpServer.h"
#include "redis/net/EventLoop.h"
#include "redis/net/Poller.h"

#include <iostream>
#include <charconv>
#include <cstdio>
#include <exception>
#include <memory>
#include <string_view>
#include <system_error>

int main(int argc, char* argv[]){
    // Read and validate the port

  int port = 6379;
  if(argc > 2){
    std::fprintf(
      stderr,
      "Usage: %s [port]\n",
      argv[0]
    );
    return 1;
  }

  if (argc == 2){
    std::string_view arg(argv[1]);
    auto [ptr, ec] = std:: from_chars(
      arg.data(),
      arg.data() + arg.size(),
      port
    );

    if(ec != std::errc{} || 
        ptr != arg.data() + arg.size() ||
        port < 1 || port > 65535){
      std::fprintf(stderr, "Invalid port\n");
      return 1;
    }
  }

  try{
    // TODO 2: Create Non blocking TCP listener
    redis::net::TcpServer server(port);

    // TODO 3: Create I/O multiplexer
    auto poller = redis::net::createPoller();

    // TODO 4: Start the single-threaded event loop.
    redis::net::EventLoop loop(server, std::move(poller));
    std::printf("Redis learning server listening on port %d\n", port);
    loop.run();

  } catch(const std::exception& e){
    std::fprintf(stderr, "Server error: %s\n", e.what());
    return 1;
  }
  return 0;
}
