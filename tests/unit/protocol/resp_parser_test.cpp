#include "redis/protocol/resp/RespEncoder.h"
#include "redis/protocol/resp/RespParser.h"
#include "redis/protocol/resp/Resp2Codec.h"

#include <cassert>
#include <iostream>
#include <string>
#include <variant>

using namespace redis::protocol::resp;

int main() {
  RespParser parser;

  // Test 1: simple string
  {
    auto result = parser.parseOne("+OK\r\n");

    assert(result.status == ParseStatus::Complete);
    assert(result.consumed == 5);

    auto* value = std::get_if<redis::protocol::SimpleString>(&result.value.data);
    assert(value != nullptr);
    assert(value->value == "OK");
  }

  // Test 2: integer.
  {
    auto result = parser.parseOne(":42\r\n");
    assert(result.status == ParseStatus::Complete);

    auto* value = std::get_if<redis::protocol::Integer>(&result.value.data);
    assert(value != nullptr);
    assert(value-> value == 42);
  }

  // Test 3: incomplete bulk string
  {
    auto result = parser.parseOne("$5\r\nhel");
    assert(result.status == ParseStatus::Incomplete);
  }

  // Test 4: binary-safe bulk string.
  {
    std::string input("$3\r\na");
    input.push_back('\0');
    input += "b\r\n";

    auto result = parser.parseOne(input);

    assert(result.status == ParseStatus::Complete);

    auto* value = std::get_if<redis::protocol::BulkString>(&result.value.data);
    assert(value != nullptr);
    assert(value->value.has_value());
    assert(value->value->size() == 3);
    assert((*value->value)[1] == '\0');
  }

  // Test 5: malformed prefix
  {
    auto result = parser.parseOne("?bad\r\n");
    assert(result.status == ParseStatus::Error);
  }

  // Test 6: array containing two bulk strings.
  {
    const std::string input =
      "*2\r\n"
      "$3\r\nGET\r\n"
      "$3\r\nkey\r\n";

    auto result = parser.parseOne(input);
    assert(result.status == ParseStatus::Complete);
    assert(result.consumed == input.size());

    auto* array = std::get_if<redis::protocol::RespValue::Array>(&result.value.data);
    assert(array != nullptr);
    assert(array->has_value());
    assert((*array)->size() == 2);
  }

  // Test 7: encoder output.
  {
    RespEncoder encoder;
    redis::protocol::RespValue value{redis::protocol::BulkString{std::string("hello")}};
    assert(encoder.encode(value) == "$5\r\nhello\r\n");
  }

  // Test 8: RESP2 codec converts a command to application args.
  {
    Resp2Codec codec;

    const std::string command = 
      "*2\r\n"
      "$3\r\nGET\r\n"
      "$3\r\nkey\r\n";

    auto result = codec.decodeOne(command);
    std::cout << "Parser status: "
              << static_cast<int>(result.status)
              << '\n';

    std::cout << "Consumed bytes: "
              << result.consumed
              << '\n';
    assert(result.status == redis::protocol::DecodeStatus::Complete);
    assert(result.request.args.size() == 2);
    assert(result.request.args[0] == "GET");
    assert(result.request.args[1] == "key");
  }
  std::cout << "Protocol tests passed\n";
};
