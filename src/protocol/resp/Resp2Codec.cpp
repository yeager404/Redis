#include "redis/protocol/resp/Resp2Codec.h"
#include "redis/protocol/resp/RespValue.h"

#include <cstdint>
#include <stdexcept>
#include <string_view>
#include <type_traits>
#include <utility>
#include <variant>

namespace redis::protocol::resp {
  Resp2Codec::Resp2Codec(ParseLimits limits)
    : parser_(limits){}

  protocol::DecodeResult Resp2Codec::decodeOne(std::string_view input){
    ParseResult parsed = parser_.parseOne(input);
    
    if(parsed.status == ParseStatus::Incomplete) {
      return {
        protocol::DecodeStatus::Incomplete, 0, {}, {}
      };
    }

    if(parsed.status == ParseStatus::Error) {
      return {
        protocol::DecodeStatus::Error, 0 , {}, std::move(parsed.error)
      };
    }

    // Commands must arrive as arrays.
    auto* array = std::get_if<RespValue::Array>(&parsed.value.data);

    if(array == nullptr || !array -> has_value()) {
      return {protocol::DecodeStatus::Error, 0, {}, "A command must be a non-null RESP array"};
    }

    application::Request request;
    request.args.reserve((*array) -> size());

    for(const RespValue& item: **array){
      // The command protocol accepts bulk strings only.
      const auto* bulk = std::get_if<BulkString>(&item.data);

      if(bulk == nullptr || !bulk->value.has_value()) {
        return {protocol::DecodeStatus::Error, 0, {}, "Command arguments must be non-null bulk strings"};
      }

      request.args.push_back(*bulk->value);
    }

    if(request.args.empty()){
      return {
        protocol::DecodeStatus::Error, 0, {}, "Command array cannot be empty"
      };
    }

    return {
      protocol::DecodeStatus::Complete,
      parsed.consumed,
      std::move(request),
      {}
    };
  }

  std::string Resp2Codec::encode(const application::Response& response){
    RespValue wireValue;
    
    std::visit([&](const auto& item){
        using T = std::decay_t<decltype(item)>;
        
        if constexpr (std::is_same_v<T, std::monostate>){
          wireValue.data = Error{"Empty application response"};
        }

        else if constexpr (std::is_same_v<T, std::string>){
          wireValue.data = BulkString{item};
        } 
        else if constexpr (std::is_same_v<T, std::int64_t>){
          wireValue.data = Integer{item};
        }
        else if constexpr (std::is_same_v<T, application::ErrorResponse>){
          wireValue.data = Error{item.message};
        }
        else if constexpr (std::is_same_v<T, application::NullResponse>){
          wireValue.data = BulkString{std::nullopt};
        }
        else if constexpr (std::is_same_v<T, application::Response::Array>){
          std::vector<RespValue> values;
          values.reserve(item.size());

          for(const auto& child: item){

            // Reuse the same conversion rules recursively.
            application::Response childResponse = child;
            Resp2Codec childCodec;
            
            // A small helper conversion avoids depending on
            // the parser for application values.
            std::string encoded = childCodec.encode(childResponse);
            ParseResult childParsed = RespParser{}.parseOne(encoded);

            if(childParsed.status != ParseStatus::Complete){
              throw std::runtime_error("Failed to convert response array item");
            }

            values.push_back(std::move(childParsed.value));
          }

          wireValue.data = RespValue::Array{std::move(values)};
        }
    }, response.value);

    return encoder_.encode(wireValue);
  }
} // namespace redis::protocol::resp
