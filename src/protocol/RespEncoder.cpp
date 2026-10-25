#include "redis/protocol/RespEncoder.h"
#include "redis/protocol/RespValue.h"

#include <stdexcept>
#include <string>
#include <type_traits>
#include <utility>

namespace redis::protocol {
  namespace {
    void encodeValue(const RespValue& value, std::string& output){
      std::visit([&](const auto& item){
          using T = std::decay_t<decltype(item)>;
          if constexpr (std::is_same_v<T, RespSimpleString>){
            if(item.value.find_first_of("\r\n") != std::string::npos){
              throw std::invalid_argument("RESP simple strings cannot contain CR or LF");
            }

            output += "+";
            output += item.value;
            output += "\r\n";
          } else if constexpr (std::is_same_v<T, RespError>) {
              if(item.value.find_first_of("\r\n") != std::string::npos){
                throw std::invalid_argument("RESP errors cannot contain CR or LF");
              }
              output += "-";
              output += item.value;
              output += "\r\n";
          } else if constexpr (std::is_same_v<T, RespInteger>) {
            output += ":";
            output += std::to_string(item.value);
            output += "\r\n";
          } else if constexpr (std::is_same_v<T, RespBulkString>) {
              if(!item.value){
                output += "$-1\r\n";
                return;
              }

              output += "$";
              output += std::to_string(item.value->size());
              output += "\r\n";
              output.append(*item.value);
              output += "\r\n";
          } else if constexpr (std::is_same_v<T, RespArray>) {
              if(!item.values){
                output += "*-1\r\n";
                return;
              }

              output += "*";
              output += std::to_string(item.values->size());
              output += "\r\n";

              for(const auto& element: *item.values){
                encodeValue(element, output);
              }
          }
      },
        value.data);
    }
  }// namespace

  std::string RespEncoder::encode(const RespValue& value) const {
    std::string output;
    encodeValue(value, output);
    return output;
  }
} // namespace redis::protocol
