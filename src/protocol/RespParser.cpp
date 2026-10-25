#include "redis/protocol/RespParser.h"
#include "redis/protocol/RespValue.h"

#include <charconv>
#include <cstddef>
#include <cstdint>
#include <iterator>
#include <limits>
#include <optional>
#include <string_view>
#include <system_error>

namespace redis::protocol {
  namespace  {
    using Result = RespParseResult;

    bool parseInteger(std::string_view text, std::int64_t& value){
      if(text.empty()){
        return false;
      }

      auto[ptr, ec] = std::from_chars(text.data(), text.data() + text.size(), value);
      return ec == std::errc{} &&
        ptr == text.data() + text.size();
    }

    ParseStatus parseValue(std::string_view input, std::size_t& pos, std::size_t depth, const ParseLimits& limits, RespValue& value, std::string& error){
      
      if(depth > limits.maxNestingDepth) {
        error = "Maximum Nesting Depth exceeded";
        return ParseStatus::Error;
      }

      if(pos >= input.size()){
        return ParseStatus::Incomplete;
      }

      char type = input[pos++];

      auto readLine = [&]() -> std::optional<std::string_view> {
        std::size_t end = input.find("\r\n", pos);

        if(end == std::string_view::npos){
          if(input.find('\n', pos) != std::string_view::npos){
            error = "Invalid line terminator";
          }

          return std::nullopt;
        }

        auto line = input.substr(pos, end - pos);
        pos = end + 2;
        return line;
      };

      if(type == '+' || type == '-'){
        auto line = readLine();

        if(!line){
          return error.empty()
            ? ParseStatus::Incomplete
            : ParseStatus::Error;
        }

        if(type == '+'){
          value.data = RespSimpleString(std::string(*line));
        }

        else {
          value.data = RespError(std::string(*line));
        }

        return ParseStatus::Complete;
      }

      if(type == ':'){
        auto line = readLine();

        if(!line){
          return error.empty() 
            ? ParseStatus::Incomplete
            : ParseStatus::Error;
        }

        std::int64_t number;

        if(!parseInteger(*line, number)){
          error = "Invalid RESP integer";
          return ParseStatus::Error;
        }

        value.data = RespInteger{number};
        return ParseStatus::Complete;
      }

      if(type == '$' || type == '*'){
        auto line = readLine();

        if(!line) {
          return error.empty()
            ? ParseStatus::Incomplete
            : ParseStatus::Error;
        }

        std::int64_t length;

        if(!parseInteger(*line, length)){
          error = "Invalid RESP length";
          return ParseStatus::Error;
        }

        if(length == -1){
          if(type == '$'){
            value.data = RespBulkString{std::nullopt};
          } else {
            value.data = RespArray{std::nullopt};
          }
          return ParseStatus::Complete;
        }

        if(length < 0){
          error = "Invalid negative RESP length";
          return ParseStatus::Error;
        }

        if(type == '$'){
          auto size = static_cast<std::uint64_t>(length);

          if(size > limits.maxBulkStringBytes){
            error = "Bulk string exceeds configured limit";
            return ParseStatus::Error;
          }

          if(size > input.size() - pos ||
              input.size() - pos - static_cast<std::size_t>(size) < 2){
            return ParseStatus::Incomplete;
          }

          std::size_t n = static_cast<std::size_t>(size);

          if(input[pos + n] != '\r' || 
              input[pos + n + 1] != '\n'){
            error = "Invalid bulk string terminator";
            return ParseStatus::Error;
          }

          value.data = RespBulkString{std::string(input.substr(pos, n))};

          pos += n + 2;
          return ParseStatus::Complete;
        }

        if(static_cast<std::uint64_t>(length) > limits.maxArrayElements){
          error = "Array exceeds configured element limit";
          return ParseStatus::Error;
        }

        std::vector<RespValue> values;
        values.reserve(static_cast<std::size_t>(length));

        for(std::int64_t i = 0; i< length; ++i){
          RespValue element{RespSimpleString{""}};
          auto status = parseValue(input, pos, depth+1, limits, element, error);

          if(status != ParseStatus::Complete){
            return status;
          }

          values.push_back(std::move(element));
        }
        value.data = RespArray{std::move(values)};
        return ParseStatus::Complete;
      }

      error = "Unknown RESP type prefix";
      return ParseStatus::Error;
    }
  }

  RespParser::RespParser(ParseLimits limits)
    : limits_(limits) {}

  RespParseResult RespParser::parseOne(std::string_view input) const {
    RespParseResult result;
    std::size_t pos = 0;

    auto status = parseValue(input, pos, 0, limits_, result.value, result.error);
    result.status = status;
    result.consumed = status == ParseStatus::Complete ? pos : 0;

    return result;
  }
}
