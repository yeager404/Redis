#include "redis/net/TcpServer.h"

#include <arpa/inet.h>
#include <asm-generic/socket.h>
#include <cmath>
#include <cstdlib>
#include <fcntl.h>
#include <memory>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#include <cerrno>
#include <system_error>

namespace redis::net{
  namespace{
    void setNonBlocking(int fd){
      int flags = ::fcntl(fd,F_GETFL,0);
      if (flags < 0 || 
          ::fcntl( fd, F_SETFL, flags | O_NONBLOCK) < 0) {
        throw std::system_error(
            errno,
            std::generic_category(),
            "fcntl O_NONBLOCK"
        ); 
      }
    }

  }// namespace


  TcpServer::TcpServer(int port){
    fd_ = ::socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK | SOCK_CLOEXEC, 0);

    if (fd_ < 0){
      throw std::system_error(
        errno,
        std::generic_category(),
        "socket"
      );
    }

    try{
      int opt = 1;

      if(::setsockopt(
            fd_,
            SOL_SOCKET,
            SO_REUSEADDR,
            &opt,
            sizeof(opt)
            ) < 0) {
        throw std::system_error(errno, std::generic_category(), "setsockopt");
      }

      sockaddr_in addr{};
      addr.sin_family = AF_INET;
      addr.sin_addr.s_addr = htonl(INADDR_ANY);
      addr.sin_port = htons(static_cast<unsigned short>(port));

      if(::bind(fd_, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) < 0){
        throw std::system_error(errno, std::generic_category(), "bind");
      }

      if(::listen(fd_, SOMAXCONN) < 0){
        throw std::system_error(errno, std::generic_category(), "listen");
      }

      setNonBlocking(fd_);
    } catch(...){
      ::close(fd_);
      fd_ = -1;
      throw;
    }
  }

  TcpServer::~TcpServer(){
    if(fd_ < 0){
      ::close(fd_);
    }
  }

  int TcpServer::fd() const{
    return fd_;
  }

  int TcpServer::acceptClient(){
    sockaddr_in clientAddr{};
    socklen_t len = sizeof(clientAddr);

    int clientFd = ::accept4(
        fd_,
        reinterpret_cast<sockaddr*>(&clientAddr),
        &len,
        SOCK_NONBLOCK | SOCK_CLOEXEC
      );

    if(clientFd < 0){
      if(errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR) {
        return -1;
      }

      throw std::system_error(errno, std::generic_category(), "accept4");
    }
    return clientFd;
  }
}
