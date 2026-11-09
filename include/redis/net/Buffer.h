#pragma once
#include <cstddef>
#include <string>
#include <string_view>

namespace redis::net {
  class Buffer {
    public:
      void append(std::string_view data);
      std::string_view view() const noexcept;

      // Remove exactly the bytes that have consumed.
      void consume(std::size_t len);

      void clear() noexcept;

      std::size_t size() const noexcept;
      bool empty() const noexcept;

    private:
      std::string data_;
  };
}
// namespace redis::net
