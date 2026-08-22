#include <cassert>
#include <iostream>

#include "resp_encoder.hpp"

int main()
{
    assert(
        encode_resp_simple_string("PONG") ==
        "+PONG\r\n"
    );

    assert(
        encode_resp_error("unknown command") ==
        "-ERR unknown command\r\n"
    );

    assert(
        encode_resp_bulk_string("Alice") ==
        "$5\r\nAlice\r\n"
    );

    assert(
        encode_resp_bulk_string("") ==
        "$0\r\n\r\n"
    );

    assert(
        encode_resp_integer(1) ==
        ":1\r\n"
    );

    assert(
        encode_resp_null_bulk_string() ==
        "$-1\r\n"
    );

    std::cout << "RESP encoder tests passed\n";
    return 0;
}