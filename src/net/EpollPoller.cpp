#include "redis/net/Poller.h"

#include <asm-generic/errno.h>
#include <cstdint>
#include <sys/epoll.h>
#include <unistd.h>

#include <cerrno>
#include <cstring>
#include <memory>
#include <stdexcept>
#include <system_error>

namespace redis::net {
  namespace {
    class EpollPoller final : public Poller {
      public:
        
        // Constructor
        EpollPoller(){
          epollFd_ = ::epoll_create1(EPOLL_CLOEXEC);
          if(epollFd_ < 0){
            throw std::system_error(errno, std::generic_category(), "epoll_create1");
          }
        }

        // Destructor
        ~EpollPoller() override {
          if(epollFd_ >= 0){
            :: close(epollFd_);
          }
        }

        void add(int fd, std::uint32_t events) override {
          update(EPOLL_CTL_ADD, fd, events);
        }

        void modify(int fd, std::uint32_t events) override {
          update(EPOLL_CTL_MOD, fd, events);
        }

        void remove(int fd) override {
          if(::epoll_ctl(epollFd_, EPOLL_CTL_DEL, fd, nullptr) < 0  && errno != ENONET && errno != EBADF){
            throw std::system_error(errno, std::generic_category(), "epoll_ctl DEL");
          }
        }

        int wait(std::vector<PollEvent>& events, int timeoutMs) override {
          epoll_event ready[128];
          

          /* Wait for events on an epoll instance "epfd". Returns the number of
          triggered events returned in "events" buffer. Or -1 in case of
          error with the "errno" variable set to the specific error code. The
          "events" parameter is a buffer that will contain triggered
          events. The "maxevents" is the maximum number of events to be
          returned ( usually size of "events" ). The "timeout" parameter
          specifies the maximum wait time in milliseconds (-1 == infinite).

          This function is a cancellation point and therefore not marked with
          __THROW.  */
          int n = ::epoll_wait(epollFd_, ready, 128, timeoutMs);

          if(n < 0){
            if(errno == EINTR){
              return 0;
            }
            throw std::system_error(errno, std::generic_category(), "epoll_wait");
          }
          
          events.clear();

          for(int i = 0; i<n; i++){
            events.push_back({ready[i].data.fd, ready[i].events});
          }

          return n;
        }
        
      private:
        void update(int operation, int fd, std::uint32_t events) {
          epoll_event event{};
          event.events = events;
          event.data.fd = fd;


          if(::epoll_ctl(epollFd_, operation, fd, &event) < 0){
            throw std::system_error(errno, std::generic_category(), "epoll_ctl");
          }
        }

        int epollFd_ = -1;
    };
  } // namespace  
  
  std::unique_ptr<Poller> createPoller(){
    return std::make_unique<EpollPoller>();
  }
} // namespace redis::net
