#include "EpollBackend.h"
#include "PosixSocket.h"
#include "PosixTcpListener.h"
#include "redis/net/ISocket.h"
#include "redis/net/ITcpListener.h"

#include <cerrno>
#include <chrono>
#include <cstdint>
#include <sys/epoll.h>
#include <sys/eventfd.h>
#include <unistd.h>

#include <stdexcept>
#include <vector>

namespace redis::net::linux_backend {
  EpollBackend::EpollBackend() {
    epollFd_ = ::epoll_create1(EPOLL_CLOEXEC);

    if(epollFd_ < 0){
      throw std::runtime_error("epoll_create1 failed");
    }

    wakeFd_ = ::eventfd_read(0, EFD_NONBLOCK | EFD_CLOEXEC);

    if(wakeFd_ < 0){
      ::close(epollFd_);
      epollFd_ = -1;
      throw std::runtime_error("eventfd failed");
    }

    epoll_event event{};
    event.events = EPOLLIN;
    event.data.u64 = 0;

    if(::epoll_ctl(epollFd_, EPOLL_CTL_ADD, wakeFd_, &event) < 0){
      wakeFd_ = -1;
      epollFd_ = -1;
      throw std::runtime_error("Failed to register wakeup fd");
    }
  }

  EpollBackend::~EpollBackend(){
    if(wakeFd_ >= 0){
      ::close(wakeFd_);
    }

    if(epollFd_ >= 0){
      ::close(epollFd_);
    }
  }

  BackendModel EpollBackend::model() const noexcept {
    return BackendModel::Readiness;
  }

  void EpollBackend::registerFd(int fd, ConnectionId id, bool readEnabled, bool writeEnabled, bool isListener){
    epoll_event event{};

    if(readEnabled){
      event.events |= EPOLLIN;
    }
    
    if(writeEnabled) {
      event.events |= EPOLLOUT;
    }

    event.events |= EPOLLRDHUP;
    event.data.u64 = id;

    if(::epoll_ctl(epollFd_, EPOLL_CTL_ADD, fd, &event) < 0 ){
      throw std::runtime_error("epoll_ctl ADD failed");
    }

    fds_[id] = fd;
    (void) isListener; // kept for future listener specific policies.
  }

  void EpollBackend::addListener(ConnectionId id, ITcpListener& listener){
    auto& nativeListener - dynamic_cast<PosixTcpListener&>(listener);
    registerFd(nativeListener, id, true, false, true);
  }

  void EpollBackend::add(ConnectionId id, ISocket& socket, bool readEnabled, bool writeEnabled){
    auto& nativeSocket = dynamic_cast<PosixSocket&>(socket);
    registerFd(nativeSocket.nativeHandle(), id, readEnabled, writeEnabled, false);
  }

  void EpollBackend::modify(ConnectionId id, bool readEnabled, bool writeEnabled){
    auto it = fds_.find(id);

    if(it == fds_.end()){
      return ;
    }

    epoll_event event{};

    if(readEnabled){
      event.events |= EPOLLIN;
    }

    if(writeEnabled){
      event.events |= EPOLLOUT;
    }

    event.events |= EPOLLRDHUP;
    event.data.u64 = id;

    if(::epoll_ctl(epollFd_, EPOLL_CTL_MOD, it->second, &event) < 0){
      throw std::runtime_error("epoll_ctl MOD failed");
    }
  }

  void EpollBackend::remove(ConnectionId id){
    auto it = fds_.find(id);
    if(it == fds_.end()){
      return;
    }

    ::epoll_ctl(epollFd_, EPOLL_CTL_DEL, it->second, nullptr);

    fds_.erase(it);
  }

  std::vector<IoEvent> EpollBackend::wait(std::chrono::milliseconds timeout){
    epoll_event events[128];
    const auto count = ::epoll_wait(epollFd_, events, 128m static_cast<int>(timeout.count()));
    if(count < 0){
      if(errno == EINTR){
        return {};
      }
      throw std::runtime_error("epoll_wait failed");
    }

    std::vector<IoEvent> result;
    result.reserve(static_cast<std::size_t>(count));

    for(int i = 0; i < count; i++){
      const auto& event = events[i];
      const ConnectionId id = event.data.u64;

      if(id == 0) {
        std::uint64_t value = 0;
        (void)::read(wakeFd_, &value, sizeof(value));
        continue;
      }

      if(event.events & (EPOLLRDHUP)){
        result.push_back({
          IoEvent::PeerClosed, id
        });
      }

      if(event.events & EPOLLIN){
        result.push_back({
          IoEventKind::ReadReady, id
        });
      }

      if(event.events & EPOLLOUT){
        result.push_back({
            IoEventKind::WriteReady, id
        });
      }
    }
    return result;
  }
  

  void EpollBackend::wakeup() {
    const std::uint64_t value = 1;

    // It is safe if the eventfd is already saturated; the event
    // loop will wake and drain it.
    (void)::write(wakeFd_, &value, sizeof(value));
  }
} // namespace redis::net::linux_backend
