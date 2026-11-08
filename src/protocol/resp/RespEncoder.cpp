#include "redis/protocol/resp/RespEncoder.h"

#include <stdexcept>
#include <type_traits>

namespace redis::protocol::resp {
namespace {

void appendEncoded(std::string& output, const RespValue& value) {
    std::visit(
        [&](const auto& item) {
            using T = std::decay_t<decltype(item)>;

            if constexpr (std::is_same_v<T, std::monostate>) {
                throw std::invalid_argument(
                    "Cannot encode an empty RespValue"
                );
            } else if constexpr (std::is_same_v<T, SimpleString>) {
                // Simple strings cannot contain line breaks because
                // CRLF terminates the value.
                if (item.value.find_first_of("\r\n") !=
                    std::string::npos) {
                    throw std::invalid_argument(
                        "Simple string contains CR or LF"
                    );
                }

                output += "+" + item.value + "\r\n";
            } else if constexpr (std::is_same_v<T, Error>) {
                if (item.value.find_first_of("\r\n") !=
                    std::string::npos) {
                    throw std::invalid_argument(
                        "Error contains CR or LF"
                    );
                }

                output += "-" + item.value + "\r\n";
            } else if constexpr (std::is_same_v<T, Integer>) {
                output += ":" + std::to_string(item.value) + "\r\n";
            } else if constexpr (std::is_same_v<T, BulkString>) {
                if (!item.value.has_value()) {
                    output += "$-1\r\n";
                } else {
                    output += "$";
                    output += std::to_string(item.value->size());
                    output += "\r\n";

                    // std::string is binary-safe. This also handles
                    // embedded null bytes.
                    output += *item.value;
                    output += "\r\n";
                }
            } else if constexpr (
                std::is_same_v<T, RespValue::Array>
            ) {
                if (!item.has_value()) {
                    output += "*-1\r\n";
                } else {
                    output += "*";
                    output += std::to_string(item->size());
                    output += "\r\n";

                    for (const RespValue& child : *item) {
                        appendEncoded(output, child);
                    }
                }
            }
        },
        value.data
    );
}

} // namespace

std::string RespEncoder::encode(const RespValue& value) const {
    std::string output;
    appendEncoded(output, value);
    return output;
}

} // namespace redis::protocol::resp
