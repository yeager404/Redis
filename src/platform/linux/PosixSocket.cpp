#include "PosixSocket.h"

#include <cerrno>
#include <cstddef>
#include <memory>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

namespace redis::net::linux_backend {
  PosixSocket::PosixSocket(int fd): fd_(fd){}

  PosixSocket::~PosixSocket(){
    close();
  }

  int PosixSocket::nativeHandler() const noexcept{
    return fd_;
  }

  IoResult PosixSocket::read (std::span<std::byte> buffer){
    if(fd_ < 0){
      return {IoStatus::Error, 0, EBADF};
    }

    const ssize_t count = ::recv(fd_, buffer.data(), buffer.size(), 0);

    if(count > 0) {
      return {
        IoStatus::Progress,
        static_cast<std::size_t>(count),
        0
      };
    }

    if(count  == 0){
      return {IoStatus::PeerClosed, 0, 0};
    }

    if(errno == EAGAIN || errno == EWOULDBLOCK) {
      return {IoStatus::WouldBlock, 0, errno};
    }

    if(errno == EINTR) {
      return {IoStatus::WouldBlock, 0, errno};
    }

    return {IoStatus::Error, 0, errno};
  }

  IoResult PosixSocket::write(std::span<const std::byte> buffer){
    if(fd_ < 0){
      return {IoStatus::Error, 0, EBADF};
    }

    // MSG_NOSIGNAL prevents a broken peer from terminating
    // the entire process with the SIGPIPE on Linux.
    const ssize_t count = ::send(fd_, buffer.data(), buffer.size(), MSG_NOSIGNAL);

    if(count > 0) {
      return {
        IoStatus::Progress,
        static_cast<std::size_t>(count),
        0
      };
    }

    if(count == 0){
      return {IoStatus::WouldBlock, 0, 0};
    }

    if(errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR) {
      return {IoStatus::WouldBlock, 0, errno};
    }

    return {IoStatus::Error, 0, error};
  }

  void PosixSocket::close() noexcept {
    if(fd_ >= 0){
      ::close(fd_);
      fd_ = -1;
    }
  }
} // namespace redis::net::linux_backend
