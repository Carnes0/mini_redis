#include "mini_redis/protocol/resp.hpp"
#include "test_support.hpp"

int main() {
    mini_redis::RespParser parser;
    auto complete = parser.parse("*2\r\n$4\r\nPING\r\n$5\r\nhello\r\n");
    CHECK_EQ(complete.status, mini_redis::ParseStatus::complete);
    CHECK_EQ(complete.request.arguments.size(), 2U);
    CHECK_EQ(complete.request.arguments.at(1), "hello");
    CHECK_EQ(complete.consumed, 25U);

    auto partial = parser.parse("*2\r\n$4\r\nPING\r\n$5\r\nhel");
    CHECK_EQ(partial.status, mini_redis::ParseStatus::incomplete);
    auto malformed = parser.parse("!2\r\n");
    CHECK_EQ(malformed.status, mini_redis::ParseStatus::protocol_error);

    std::string pipelined = "*1\r\n$4\r\nPING\r\n";
    pipelined.append(mini_redis::max_request_size, 'x');
    auto first_frame = parser.parse(pipelined);
    CHECK_EQ(first_frame.status, mini_redis::ParseStatus::complete);
    CHECK_EQ(first_frame.request.arguments.at(0), "PING");

    CHECK_EQ(mini_redis::encode(mini_redis::SimpleString{"PONG"}), "+PONG\r\n");
    CHECK_EQ(mini_redis::encode(mini_redis::ErrorString{"ERR bad"}), "-ERR bad\r\n");
    CHECK_EQ(mini_redis::encode(mini_redis::Integer{2}), ":2\r\n");
    CHECK_EQ(mini_redis::encode(mini_redis::BulkString{"hi"}), "$2\r\nhi\r\n");
    CHECK_EQ(mini_redis::encode(mini_redis::NullBulkString{}), "$-1\r\n");
    return test::finish();
}
