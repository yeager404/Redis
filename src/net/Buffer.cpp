#include "redis/net/Buffer.h"
#include <algorithm>
#include <cstddef>
#include <iterator>
#include <stdexcept>
#include <string_view>

namespace redis::net {
  void Buffer::append(std::string_view data){
    data_.append(data.data(), data.size());
  }

  std::string_view Buffer::view() const noexcept {
    return data_;
  }

  void Buffer::consume(std::size_t count){
    if(count > data_.size()){
      throw std::out_of_range("Buffer consume exceeds size");
    }

    data_.erase(0, count);
  }

  void Buffer::clear(){
    data_.clear();
  }
  
  std::size_t Buffer::size() const noexcept{
    return data_.size();
  }

  bool Buffer::empty() const noexcept {
    return data_.empty();
  }

}// namespace redis::net
