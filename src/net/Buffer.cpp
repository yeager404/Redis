#include "redis/net/Buffer.h"
#include <algorithm>
#include <cstddef>
#include <iterator>

namespace redis::net {
  void Buffer::append(const char*data, std::size_t len){
    data_.append(data, len);
  }

  const char* Buffer::readData() const {
    return data_.data() + readPos_;
  }

  std::size_t Buffer::readableBytes() const {
    return data_.size() - readPos_;
  }

  void Buffer::consume(std::size_t len){
    readPos_ += std::min(len, readableBytes());

    if(readPos_ == data_.size()){
      clear();
    } else if(readPos_ > 4096 && readPos_ > data_.size() / 2){
      data_.erase(0, readPos_);
      readPos_ = 0;
    }
  }

  void Buffer::clear(){
    data_.clear();
    readPos_ = 0;
  }

}// namespace redis::net
