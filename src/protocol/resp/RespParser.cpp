#include "redis/protocol/resp/RespParser.h"

#include <charconv>
#include <limits>
#include <utility>

namespace redis::protocol::resp {
namespace {

// Internal result used by recursive parsing.
struct InternalResult {
    ParseStatus status = ParseStatus::Incomplete;
    std::size_t consumed = 0;
    RespValue value;
    std::string error;
};

// Convert an internal result into the public result.
ParseResult publicResult(InternalResult result) {
    return {
        result.status,
        result.consumed,
        std::move(result.value),
        std::move(result.error)
    };
}

// A RESP line ends in CRLF. Return the position of CR,
// or report incomplete input if the terminator has not arrived.
bool findLineEnd(
    std::string_view input,
    std::size_t start,
    std::size_t& end
) {
    for (std::size_t i = start; i < input.size(); ++i) {
        if (input[i] == '\n') {
            // A bare LF is invalid in RESP2.
            if (i == 0 || input[i - 1] != '\r') {
                return false;
            }

            end = i - 1; // Position of CR.
            return true;
        }
    }

    return false;
}

// Parse a decimal integer from a string.
// from_chars avoids locale-dependent parsing.
bool parseInt64(std::string_view text, std::int64_t& value) {
    if (text.empty()) {
        return false;
    }

    const char* begin = text.data();
    const char* end = text.data() + text.size();

    auto [ptr, error] = std::from_chars(begin, end, value);

    return error == std::errc{} && ptr == end;
}

bool parseSize(std::string_view text, std::size_t& value) {
    std::uint64_t parsed = 0;

    if (text.empty()) {
        return false;
    }

    const char* begin = text.data();
    const char* end = text.data() + text.size();

    auto [ptr, error] = std::from_chars(begin, end, parsed);

    if (error != std::errc{} || ptr != end) {
        return false;
    }

    if (parsed > std::numeric_limits<std::size_t>::max()) {
        return false;
    }

    value = static_cast<std::size_t>(parsed);
    return true;
}

// Parse one RESP value recursively.
//
// `start` is the first byte of the value.
// `depth` prevents deeply nested arrays from exhausting the stack.
InternalResult parseValue(
    std::string_view input,
    std::size_t start,
    std::size_t depth,
    const ParseLimits& limits
) {
    if (start >= input.size()) {
        return {};
    }

    if (depth > limits.maxNestingDepth) {
        return {
            ParseStatus::Error, 0, {},
            "Maximum RESP nesting depth exceeded"
        };
    }

    const char prefix = input[start];

    // Find the CRLF terminator for line-based values.
    std::size_t lineEnd = 0;
    const bool hasLine = findLineEnd(input, start + 1, lineEnd);

    // Simple strings, errors, integers, and array/bulk lengths
    // all begin with a line-based header.
    if (!hasLine) {
        // Distinguish incomplete input from a malformed bare LF.
        for (std::size_t i = start + 1; i < input.size(); ++i) {
            if (input[i] == '\n' &&
                (i == 0 || input[i - 1] != '\r')) {
                return {
                    ParseStatus::Error, 0, {},
                    "RESP line contains bare LF"
                };
            }
        }

        return {};
    }

    const std::string_view line =
        input.substr(start + 1, lineEnd - (start + 1));

    // Number of bytes in the header, including CRLF.
    const std::size_t headerBytes = lineEnd + 2 - start;

    switch (prefix) {
        case '+': {
            return {
                ParseStatus::Complete,
                headerBytes,
                RespValue{SimpleString{std::string(line)}},
                {}
            };
        }

        case '-': {
            return {
                ParseStatus::Complete,
                headerBytes,
                RespValue{Error{std::string(line)}},
                {}
            };
        }

        case ':': {
            std::int64_t number = 0;

            if (!parseInt64(line, number)) {
                return {
                    ParseStatus::Error, 0, {},
                    "Invalid RESP integer"
                };
            }

            return {
                ParseStatus::Complete,
                headerBytes,
                RespValue{Integer{number}},
                {}
            };
        }

        case '$': {
            // Bulk string header: $<length>\r\n
            std::int64_t signedLength = 0;

            if (!parseInt64(line, signedLength)) {
                return {
                    ParseStatus::Error, 0, {},
                    "Invalid bulk string length"
                };
            }

            // RESP2 permits -1 for a null bulk string.
            if (signedLength == -1) {
                return {
                    ParseStatus::Complete,
                    headerBytes,
                    RespValue{BulkString{std::nullopt}},
                    {}
                };
            }

            if (signedLength < 0) {
                return {
                    ParseStatus::Error, 0, {},
                    "Invalid negative bulk string length"
                };
            }

            const auto length =
                static_cast<std::uint64_t>(signedLength);

            if (length > limits.maxBulkStringBytes) {
                return {
                    ParseStatus::Error, 0, {},
                    "Bulk string exceeds configured limit"
                };
            }

            const std::size_t payloadStart = lineEnd + 2;

            // Check bounds before adding length to payloadStart.
            if (length > input.size() - payloadStart) {
                return {};
            }

            const std::size_t payloadLength =
                static_cast<std::size_t>(length);

            const std::size_t terminator =
                payloadStart + payloadLength;

            // Payload must be followed by CRLF.
            if (input.size() - terminator < 2) {
                return {};
            }

            if (input[terminator] != '\r' ||
                input[terminator + 1] != '\n') {
                return {
                    ParseStatus::Error, 0, {},
                    "Bulk string is missing CRLF terminator"
                };
            }

            std::string payload(
                input.substr(payloadStart, payloadLength)
            );

            return {
                ParseStatus::Complete,
                terminator + 2 - start,
                RespValue{BulkString{std::move(payload)}},
                {}
            };
        }

        case '*': {
            // Array header: *<element-count>\r\n
            std::int64_t signedCount = 0;

            if (!parseInt64(line, signedCount)) {
                return {
                    ParseStatus::Error, 0, {},
                    "Invalid array length"
                };
            }

            // RESP2 permits -1 for a null array.
            if (signedCount == -1) {
                return {
                    ParseStatus::Complete,
                    headerBytes,
                    RespValue{RespValue::Array{std::nullopt}},
                    {}
                };
            }

            if (signedCount < 0) {
                return {
                    ParseStatus::Error, 0, {},
                    "Invalid negative array length"
                };
            }

            const auto count =
                static_cast<std::uint64_t>(signedCount);

            if (count > limits.maxArrayElements) {
                return {
                    ParseStatus::Error, 0, {},
                    "Array exceeds configured element limit"
                };
            }

            std::vector<RespValue> values;
            values.reserve(static_cast<std::size_t>(count));

            // Start after the array header.
            std::size_t position = start + headerBytes;

            for (std::size_t i = 0;
                 i < static_cast<std::size_t>(count);
                 ++i) {

                InternalResult child =
                    parseValue(input, position, depth + 1, limits);

                if (child.status != ParseStatus::Complete) {
                    // The whole array is incomplete if any child
                    // is incomplete. The caller will retry later.
                    return {
                        child.status, 0, {}, std::move(child.error)
                    };
                }

                if (child.consumed == 0) {
                    return {
                        ParseStatus::Error, 0, {},
                        "Parser made no progress"
                    };
                }

                position += child.consumed;
                values.push_back(std::move(child.value));
            }

            return {
                ParseStatus::Complete,
                position - start,
                RespValue{
                    RespValue::Array{std::move(values)}
                },
                {}
            };
        }

        default:
            return {
                ParseStatus::Error, 0, {},
                "Unknown RESP type prefix"
            };
    }
}

} // namespace

RespParser::RespParser(ParseLimits limits)
    : limits_(limits) {
}

ParseResult RespParser::parseOne(
    std::string_view input
) const {
    return publicResult(parseValue(input, 0, 0, limits_));
}

} // namespace redis::protocol::resp
