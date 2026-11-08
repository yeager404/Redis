#include "redis/net/Buffer.h"

#include <cassert>
#include <cstring>
#include <iostream>
#include <string>

using redis::net::Buffer;

int main() {
    // Test 1: Empty buffer behavior
    {
        Buffer buf;
        assert(buf.readableBytes() == 0);
        assert(buf.readData() != nullptr);
        buf.clear();
        assert(buf.readableBytes() == 0);
    }

    // Test 2: Append and read data
    {
        Buffer buf;
        const char* msg = "hello redis";
        buf.append(msg, std::strlen(msg));

        assert(buf.readableBytes() == std::strlen(msg));
        assert(std::string_view(buf.readData(), buf.readableBytes()) == "hello redis");
    }

    // Test 3: Partial consumption
    {
        Buffer buf;
        buf.append("hello world", 11);

        buf.consume(6); // consume "hello "
        assert(buf.readableBytes() == 5);
        assert(std::string_view(buf.readData(), buf.readableBytes()) == "world");

        buf.consume(5); // consume "world"
        assert(buf.readableBytes() == 0);
    }

    // Test 4: Over-consumption clamps to readableBytes
    {
        Buffer buf;
        buf.append("test", 4);
        buf.consume(100);
        assert(buf.readableBytes() == 0);
    }

    // Test 5: Exact consumption clears buffer
    {
        Buffer buf;
        buf.append("redis", 5);
        buf.consume(5);
        assert(buf.readableBytes() == 0);

        // Appending again works cleanly from offset 0
        buf.append("next", 4);
        assert(buf.readableBytes() == 4);
        assert(std::string_view(buf.readData(), buf.readableBytes()) == "next");
    }

    // Test 6: Compaction threshold test (readPos_ > 4096 && readPos_ > data_.size() / 2)
    {
        Buffer buf;
        // Append 10,000 bytes
        std::string payload(10000, 'x');
        for (std::size_t i = 0; i < payload.size(); ++i) {
            payload[i] = static_cast<char>('a' + (i % 26));
        }
        buf.append(payload.data(), payload.size());

        // Consume 5001 bytes (greater than 4096 and greater than 10000 / 2)
        buf.consume(5001);

        assert(buf.readableBytes() == 10000 - 5001);
        std::string_view expected(payload.data() + 5001, 10000 - 5001);
        assert(std::string_view(buf.readData(), buf.readableBytes()) == expected);
    }

    // Test 7: Interleaved appends and consumes
    {
        Buffer buf;
        buf.append("chunk1-", 7);
        assert(buf.readableBytes() == 7);

        buf.append("chunk2-", 7);
        assert(buf.readableBytes() == 14);

        buf.consume(7);
        assert(std::string_view(buf.readData(), buf.readableBytes()) == "chunk2-");

        buf.append("chunk3", 6);
        assert(std::string_view(buf.readData(), buf.readableBytes()) == "chunk2-chunk3");

        buf.consume(13);
        assert(buf.readableBytes() == 0);
    }

    std::cout << "All Buffer tests passed successfully!\n";
    return 0;
}
