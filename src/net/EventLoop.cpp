#include "redis/net/EventLoop.h"

#include <cstdint>
#include <memory>
#include <sys/epoll.h>
#include <sys/socket.h>

#include <cerrno>
#include <cstdio>
#include <cstring>
#include <stdexcept>
#include <sys/types.h>
#include <system_error>
#include <vector>



/**
 * Note: events are basically 32 bit integer constants, with only 1 set bit.
 *        the kernel will understand th operation based on what bit is set.
 */


namespace redis::net {
  EventLoop::EventLoop(TcpServer& server, std::unique_ptr<Poller> poller)
    : server_(server),
      poller_(std::move(poller)) {
        if(!poller_){
          throw std::invalid_argument("Poller cannot be null");
        }

        // Register the server's listening file descriptor with OS kernel
        // EPOLLIN -> wake me up when there's incoming data
        poller_->add(server_.fd(), EPOLLIN); 
      }

  void EventLoop::run(){
    std::vector<PollEvent> events;
    std::printf("Event loop started\n");
    
    while(running_){
      // wait for infinite time, untill interrupt
      // poller will wait indefinitely, when it wakes up, events will be populated
      poller_->wait(events, -1);
      
      // a reference to a pollevent -> contains a FD, a bitmask of what happened  
      for(const auto&event: events){

        // if fd that woke up is the main server listening socket, then route it to
        // Connection-acceptance logic

        if(event.fd == server_.fd()){
          handleAccept();
        } 
        // Otherwise the event belongs to an already connected client.
        else {
          handleClient(event.fd, event.events);
        }
      }
    }
  }

  void EventLoop::handleAccept() {
    while(true) {
      int fd = server_.acceptClient();

      if(fd < 0){
        break;
      }

      connections_.add(fd);

      try{
        // EPOLLRDHUP -> wake me if the client gracefully closes the connection.
        poller_ -> add(fd, EPOLLIN | EPOLLRDHUP);
      } catch(...){
        connections_.remove(fd);
        throw;
      }

      std::printf("Client connected: fd=%d\n", fd);
    }
  }

  void EventLoop::handleClient(
    int fd,
    unsigned int events
) {
    Connection* conn = connections_.get(fd);

    if (!conn) {
        std::printf("[EVENT] Unknown fd=%d\n", fd);
        return;
    }

//    std::printf(
//    "[EVENT] fd=%d flags=0x%x\n",
//        fd, events
//    );

    if (events & EPOLLERR) {
        int err = 0;
        socklen_t len = sizeof(err);

        ::getsockopt(
            fd, SOL_SOCKET, SO_ERROR, &err, &len
        );

 //       std::printf("[ERROR] fd=%d socket error=%d\n", fd, err);
        closeClient(fd);
        return;
    }

    if (events & (EPOLLIN | EPOLLRDHUP)) {
        if (!readClient(*conn)) {
            std::printf("[CLOSE] readClient failed, fd=%d\n", fd);
            closeClient(fd);
            return;
        }
    }

    if (conn->outputBuffer().readableBytes() > 0) {
        if (!writeClient(*conn)) {
            std::printf("[CLOSE] writeClient failed, fd=%d\n", fd);
            closeClient(fd);
            return;
        }
    }

    if (events & EPOLLHUP) {
        std::printf("[CLOSE] EPOLLHUP, fd=%d\n", fd);
        closeClient(fd);
        return;
    }

    if (conn->peerClosed() &&
        conn->outputBuffer().readableBytes() == 0) {
        std::printf("[CLOSE] Peer closed; output drained, fd=%d\n", fd);
        closeClient(fd);
        return;
    }

    updateInterest(*conn);
}
bool EventLoop::readClient(Connection& conn) {
    char buffer[4096];

    while (true) {
        ssize_t n = ::recv(
            conn.fd(),
            buffer,
            sizeof(buffer),
            0
        );

        if (n > 0) {
//            std::printf("[RECV] fd=%d bytes=%zd\n", conn.fd(), n);

            auto len = static_cast<std::size_t>(n);

            conn.inputBuffer().append(buffer, len);

            conn.outputBuffer().append(
                conn.inputBuffer().readData(),
                conn.inputBuffer().readableBytes()
            );

            conn.inputBuffer().consume(
                conn.inputBuffer().readableBytes()
            );

            continue;
        }

        if (n == 0) {
            std::printf(
                "[RECV] fd=%d returned 0: peer closed writing\n",
                conn.fd()
            );

            conn.markPeerClosed();
            return true;
        }

        if (errno == EINTR) {
            continue;
        }

        if (errno == EAGAIN || errno == EWOULDBLOCK) {
//            std::printf("[RECV] fd=%d drained; waiting for more data\n", conn.fd());
            return true;
        }

        std::printf(
            "[RECV ERROR] fd=%d errno=%d: %s\n",
            conn.fd(), errno, strerror(errno)
        );

        return false;
    }
}

  bool EventLoop::writeClient(Connection& conn) {
    Buffer& output = conn.outputBuffer();

    while(output.readableBytes() > 0){
      // MSG_NOSIGNAL -> suppreses a crash and safely returns -1 , with errno = EPIPE
      ssize_t n = ::send(conn.fd(), output.readData(), output.readableBytes(), MSG_NOSIGNAL);

      if(n > 0){
        // shifts the sliding window of your Buffer.
        output.consume(static_cast<std::size_t>(n));
        continue;
      }

      if(n < 0 && errno == EINTR){
        continue;
      }

      if(n < 0 && (errno == EAGAIN || errno == EWOULDBLOCK)){
        return true;
      }

      return false;
    }
    return true;
  }

  void EventLoop::updateInterest(Connection&conn){
    std::uint32_t events = EPOLLIN | EPOLLRDHUP;

    if(conn.outputBuffer().readableBytes() > 0){
      events |= EPOLLOUT;
    }

    poller_->modify(conn.fd(), events);
  }

  void EventLoop::closeClient(int fd){
    poller_->remove(fd);
    connections_.remove(fd);

    std::printf("Client disconnected: fd=%d\n", fd);
  }
} // namespace redis::net
