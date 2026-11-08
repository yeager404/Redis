#include "redis/application/Request.h"
#include "redis/application/Response.h"
#include "redis/protocol/resp/Resp2Codec.h"
#include "redis/protocol/resp/Resp2ProtocolFactory.h"
#include "redis/protocol/resp/RespEncoder.h"
#include "redis/protocol/resp/RespParser.h"
#include "redis/protocol/resp/RespValue.h"

#include <cassert>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

using namespace redis::protocol;
using namespace redis::protocol::resp;
using namespace redis::application;

int main() {
    // -------------------------------------------------------------
    // Section 1: RespEncoder Tests
    // -------------------------------------------------------------
    {
        RespEncoder encoder;

        // SimpleString
        RespValue ss{SimpleString{"PONG"}};
        assert(encoder.encode(ss) == "+PONG\r\n");

        // SimpleString rejecting CR / LF
        try {
            RespValue invalidSS{SimpleString{"BAD\r\nSTRING"}};
            encoder.encode(invalidSS);
            assert(false && "Should have thrown invalid_argument");
        } catch (const std::invalid_argument&) {}

        // Error
        RespValue err{Error{"ERR unknown command"}};
        assert(encoder.encode(err) == "-ERR unknown command\r\n");

        // Error rejecting CR / LF
        try {
            RespValue invalidErr{Error{"BAD\nERROR"}};
            encoder.encode(invalidErr);
            assert(false && "Should have thrown invalid_argument");
        } catch (const std::invalid_argument&) {}

        // Integer
        RespValue intZero{Integer{0}};
        assert(encoder.encode(intZero) == ":0\r\n");

        RespValue intNeg{Integer{-12345}};
        assert(encoder.encode(intNeg) == ":-12345\r\n");

        // BulkString
        RespValue bulkEmpty{BulkString{std::string("")}};
        assert(encoder.encode(bulkEmpty) == "$0\r\n\r\n");

        RespValue bulkNull{BulkString{std::nullopt}};
        assert(encoder.encode(bulkNull) == "$-1\r\n");

        std::string binaryPayload("bin");
        binaryPayload.push_back('\0');
        binaryPayload.append("data\r\nend");
        RespValue bulkBinary{BulkString{binaryPayload}};
        std::string expectedBinary = "$" + std::to_string(binaryPayload.size()) + "\r\n" + binaryPayload + "\r\n";
        assert(encoder.encode(bulkBinary) == expectedBinary);

        // Array
        RespValue emptyArray{RespValue::Array{std::vector<RespValue>{}}};
        assert(encoder.encode(emptyArray) == "*0\r\n");

        RespValue nullArray{RespValue::Array{std::nullopt}};
        assert(encoder.encode(nullArray) == "*-1\r\n");

        // Monostate should throw
        RespValue emptyVal;
        try {
            encoder.encode(emptyVal);
            assert(false && "Should throw for empty RespValue");
        } catch (const std::invalid_argument&) {}
    }

    // -------------------------------------------------------------
    // Section 2: RespParser Limits and Edge Cases
    // -------------------------------------------------------------
    {
        // Null bulk string and null array parsing
        RespParser parser;
        auto resNullBulk = parser.parseOne("$-1\r\n");
        assert(resNullBulk.status == ParseStatus::Complete);
        auto* bs = std::get_if<BulkString>(&resNullBulk.value.data);
        assert(bs && !bs->value.has_value());

        auto resNullArray = parser.parseOne("*-1\r\n");
        assert(resNullArray.status == ParseStatus::Complete);
        auto* arr = std::get_if<RespValue::Array>(&resNullArray.value.data);
        assert(arr && !arr->has_value());

        // Bare LF without CR should report Error
        auto resBareLF = parser.parseOne("+OK\n");
        assert(resBareLF.status == ParseStatus::Error);

        // Negative length other than -1
        auto resInvalidLen = parser.parseOne("$-2\r\n");
        assert(resInvalidLen.status == ParseStatus::Error);

        // Incomplete inputs
        assert(parser.parseOne("$").status == ParseStatus::Incomplete);
        assert(parser.parseOne("$4\r\n").status == ParseStatus::Incomplete);
        assert(parser.parseOne("$4\r\nPIN").status == ParseStatus::Incomplete);
        assert(parser.parseOne("$4\r\nPING\r").status == ParseStatus::Incomplete);

        // Configured limits
        ParseLimits strictLimits;
        strictLimits.maxBulkStringBytes = 5;
        strictLimits.maxArrayElements = 2;
        strictLimits.maxNestingDepth = 2;

        RespParser strictParser(strictLimits);

        // Exceeding bulk string limit
        auto resBulkExceed = strictParser.parseOne("$10\r\n0123456789\r\n");
        assert(resBulkExceed.status == ParseStatus::Error);

        // Exceeding array elements limit
        auto resArrayExceed = strictParser.parseOne("*3\r\n:1\r\n:2\r\n:3\r\n");
        assert(resArrayExceed.status == ParseStatus::Error);

        // Exceeding nesting depth
        // Nesting depth 3 when limit is 2
        auto resNestExceed = strictParser.parseOne("*1\r\n*1\r\n*1\r\n:1\r\n");
        assert(resNestExceed.status == ParseStatus::Error);
    }

    // -------------------------------------------------------------
    // Section 3: Resp2Codec Pipelined Commands
    // -------------------------------------------------------------
    {
        Resp2Codec codec;
        std::string stream =
            "*1\r\n$4\r\nPING\r\n"
            "*2\r\n$4\r\nECHO\r\n$5\r\nhello\r\n"
            "*3\r\n$3\r\nSET\r\n$3\r\nfoo\r\n$3\r\nbar\r\n";

        std::size_t offset = 0;
        std::vector<std::vector<std::string>> commands;

        while (offset < stream.size()) {
            std::string_view remaining(stream.data() + offset, stream.size() - offset);
            auto result = codec.decodeOne(remaining);
            if (result.status != DecodeStatus::Complete) {
                break;
            }
            commands.push_back(result.request.args);
            offset += result.consumed;
        }

        assert(commands.size() == 3);
        assert(commands[0] == (std::vector<std::string>{"PING"}));
        assert(commands[1] == (std::vector<std::string>{"ECHO", "hello"}));
        assert(commands[2] == (std::vector<std::string>{"SET", "foo", "bar"}));
        assert(offset == stream.size());
    }

    // -------------------------------------------------------------
    // Section 4: Resp2Codec Validation Edge Cases
    // -------------------------------------------------------------
    {
        Resp2Codec codec;

        // Command not an array
        auto nonArray = codec.decodeOne("+PING\r\n");
        assert(nonArray.status == DecodeStatus::Error);

        // Command is null array
        auto nullArr = codec.decodeOne("*-1\r\n");
        assert(nullArr.status == DecodeStatus::Error);

        // Command is empty array
        auto emptyArr = codec.decodeOne("*0\r\n");
        assert(emptyArr.status == DecodeStatus::Error);

        // Command arguments not bulk string
        auto nonBulkArg = codec.decodeOne("*2\r\n:100\r\n$3\r\nGET\r\n");
        assert(nonBulkArg.status == DecodeStatus::Error);

        // Command arguments contain null bulk string
        auto nullBulkArg = codec.decodeOne("*2\r\n$3\r\nGET\r\n$-1\r\n");
        assert(nullBulkArg.status == DecodeStatus::Error);
    }

    // -------------------------------------------------------------
    // Section 5: Resp2Codec Response Encoding
    // -------------------------------------------------------------
    {
        Resp2Codec codec;

        // String response -> bulk string
        assert(codec.encode(Response::string("OK")) == "$2\r\nOK\r\n");

        // Integer response
        assert(codec.encode(Response::integer(42)) == ":42\r\n");

        // Error response
        assert(codec.encode(Response::error("ERR syntax error")) == "-ERR syntax error\r\n");

        // Null response -> $-1\r\n
        assert(codec.encode(Response::null()) == "$-1\r\n");

        // Array response
        Response arrRes = Response::array({
            Response::string("element1"),
            Response::integer(100),
            Response::null()
        });
        std::string expectedArr = "*3\r\n$8\r\nelement1\r\n:100\r\n$-1\r\n";
        assert(codec.encode(arrRes) == expectedArr);
    }

    // -------------------------------------------------------------
    // Section 6: Resp2ProtocolFactory
    // -------------------------------------------------------------
    {
        Resp2ProtocolFactory factory;
        auto codec = factory.createCodec();
        assert(codec != nullptr);

        auto res = codec->decodeOne("*1\r\n$4\r\nPING\r\n");
        assert(res.status == DecodeStatus::Complete);
        assert(res.request.args.size() == 1);
        assert(res.request.args[0] == "PING");
    }

    std::cout << "All RespCodec & RespEncoder tests passed successfully!\n";
    return 0;
}
