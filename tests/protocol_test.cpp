#include "redis/protocol/RespEncoder.h"
#include "redis/protocol/RespParser.h"

#include <cassert>
#include <iostream>
#include <string>
#include <variant>

using namespace redis::protocol;

int main() {
    RespParser parser;
    RespEncoder encoder;

    // TODO 1: Parse a simple string.
    {
        auto result = parser.parseOne("+OK\r\n");

        assert(result.status == ParseStatus::Complete);
        assert(result.consumed == 5);

        auto* value =
            std::get_if<RespSimpleString>(&result.value.data);

        assert(value && value->value == "OK");
    }

    // TODO 2: Parse a RESP command array.
    {
        std::string request =
            "*2\r\n"
            "$4\r\nPING\r\n"
            "$4\r\ntest\r\n";

        auto result = parser.parseOne(request);

        assert(result.status == ParseStatus::Complete);
        assert(result.consumed == request.size());

        auto* array =
            std::get_if<RespArray>(&result.value.data);

        assert(array && array->values);
        assert(array->values->size() == 2);
    }

    // TODO 3: Detect incomplete input.
    {
        auto result = parser.parseOne("$5\r\nhel");

        assert(result.status == ParseStatus::Incomplete);
        assert(result.consumed == 0);
    }

    // TODO 4: Reject an invalid type prefix.
    {
        auto result = parser.parseOne("?hello\r\n");

        assert(result.status == ParseStatus::Error);
    }

    // TODO 5: Round-trip a binary-safe bulk string.
    {
        std::string binary("a\0b", 3);

        RespValue value{
            RespBulkString{binary}
        };

        std::string wire = encoder.encode(value);
        auto result = parser.parseOne(wire);

        assert(result.status == ParseStatus::Complete);

        auto* bulk =
            std::get_if<RespBulkString>(&result.value.data);

        assert(bulk && bulk->value);
        assert(*bulk->value == binary);
    }

    // TODO 6: Preserve multiple consecutive requests.
    {
        std::string requests = "+OK\r\n:42\r\n";

        auto first = parser.parseOne(requests);

        assert(first.status == ParseStatus::Complete);

        auto second = parser.parseOne(
            std::string_view(requests).substr(first.consumed)
        );

        assert(second.status == ParseStatus::Complete);
        assert(std::get<RespInteger>(second.value.data).value == 42);
    }

    // TODO 7: Encode a simple string response.
    {
        RespValue value{
            RespSimpleString{"PONG"}
        };

        assert(encoder.encode(value) == "+PONG\r\n");
    }

    std::cout << "All RESP2 tests passed\n";
    return 0;
}
