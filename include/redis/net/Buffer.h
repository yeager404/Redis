#pragma once
#include <cstddef>
#include <string>

namespace redis::net {
  class Buffer {
    public:
      void append(const char* data, std::size_t len);

      const char* readData() const;
      std::size_t readableBytes() const;

      void consume(std::size_t len);
      void clear();

    private:
      std::string data_;
      std::size_t readPos_ = 0;
  };
}
// namespace redis::net
