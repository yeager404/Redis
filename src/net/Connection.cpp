#include "redis/net/Connection.h"
#include "redis/net/Buffer.h"
#include <unistd.h>

namespace redis::net {
  Connection::Connection(int fd)
    : fd_(fd){}

  Connection::~Connection(){
    if(fd_ >= 0){
      ::close(fd_);
    }
  }

  int Connection::fd() const{
    return fd_;
  }

  Buffer& Connection::inputBuffer(){
    return input_;
  }

  Buffer& Connection::outputBuffer(){
    return output_;
  }

  bool Connection::peerClosed() const {
    return peerClosed_;
  }

  void Connection::markPeerClosed() {
    peerClosed_ = true;
  }
}
// namespace redis::net
