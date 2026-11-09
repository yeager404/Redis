#include "PosixTcpListener.h"
#include "PosixSocket.h"

#include <asm-generic/socket.h>
#include <cerrno>
#include <cstring>
#include <fcntl.h>
#include <memory>
#include <netdb.h>
#include <stdexcept>
#include <string>
#include <sys/socket.h>
#include <unistd.h>

namespace redis::net::linux_backend {
  namespace {
    bool setNonBlocking(int fd){
      const int flags = ::fcntl(fd, F_GETFL, 0);
      return flags >= 0 && 
        ::fcntl(fd, F_SETFL, flags | O_NONBLOCK) == 0;
    }
  } // namespace
  
  PosixTcpListener::PosixTcpListener(const IoConfig& config) {
    addrinfo hints{};
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    addrinfo* addresses = nullptr;
    const std::string port = std::to_string(config.port);

    const int lookupResult = ::getaddrinfo(config.host.c_str(), port.c_str(), &hints, &addresses);

    if(lookupResult != 0){
      throw std::runtime_error(std::string("getaddrinfo: ") + gai_strerror(lookupResult));
    }

    for(addrinfo* address = addresses; address != nullptr; address = address->ai_next){
      const int candidate = ::socket(address -> ai_family, address->ai_socktype, address->ai_protocol);
      if(candidate < 0) {
        continue;
      }

      int reuse = 1;
      ::setsockopt(candidate, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));


      if(!setNonBlocking(candidate)){
        ::close(candidate);
        continue;
      }
      
      if(::bind(candidate, address->ai_addr, address->ai_addrlen) == 0 && ::listen(candidate, config.backlog) == 0 ){
        fd_ = candidate;
        break;
      }

      ::close(candidate);
    }


    if(fd_ < 0){
      throw std::runtime_error("Failed to bind/listen on configured address");
    }
  }

  PosixTcpListener::~PosixTcpListener(){
    close();
  }

  int PosixTcpListener::nativeHandle() const noexcept{
    return fd_;
  }

  AcceptResult PosixTcpListener::accept() {
    sockaddr_storage peer{};
    socklen_t peerLength = sizeof(peer);

    const int clientFd = ::accept(fd_, reinterpret_cast<sockaddr*>(&peer), &peerLength);

    if(clientFd < 0){
      if(errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR){
        return {nullptr, {IoStatus::WouldBlock, 0, errno}};
      }

      return {nullptr, {IoStatus::Error, 0, error}};
    }

    if(!setNonBlocking(clientFd)){
      const int error = errno;
      ::close(clientFd);
      return {nullptr, {IoStatus::Error, 0, error}};
    }

    return {
      std::make_unique<PosixSocket>(clientFd);
      {IoStatus::Progress, 0, 0}
    };
  }

  void PosixTcpListener::close() noexcept {
    if(fd_ >= 0 ){
      ::close(fd_);
      fd_ = -1;
    }
  }

} // namespace redis::net::linux_backend
