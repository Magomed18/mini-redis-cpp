#include <cassert>
#include <iostream>
#include <string>

#include "resp_parser.hpp"

void test_ping_command()
{
    const std::string request = "*1\r\n$4\r\nPING\r\n";
    const RespParseResult result = parse_resp_command(request);

    assert(result.status == RespParseStatus::Complete);
    assert(result.arguments.size() == 1);
    assert(result.arguments[0] == "PING");
    assert(result.bytes_consumed == request.size());
}

void test_set_command()
{
    const std::string request =
        "*3\r\n"
        "$3\r\nSET\r\n"
        "$4\r\nname\r\n"
        "$5\r\nAlice\r\n";

    const RespParseResult result = parse_resp_command(request);

    assert(result.status == RespParseStatus::Complete);
    assert(result.arguments.size() == 3);
    assert(result.arguments[0] == "SET");
    assert(result.arguments[1] == "name");
    assert(result.arguments[2] == "Alice");
}

void test_incomplete_command()
{
    const RespParseResult result =
        parse_resp_command("*1\r\n$4\r\nPI");

    assert(result.status == RespParseStatus::Incomplete);
    assert(result.bytes_consumed == 0);
}

void test_invalid_command()
{
    const RespParseResult result =
        parse_resp_command("*1\r\n+PING\r\n");

    assert(result.status == RespParseStatus::Invalid);
}

void test_one_command_consumed_at_a_time()
{
    const std::string ping = "*1\r\n$4\r\nPING\r\n";
    const std::string combined = ping + ping;

    const RespParseResult result =
        parse_resp_command(combined);

    assert(result.status == RespParseStatus::Complete);
    assert(result.arguments[0] == "PING");
    assert(result.bytes_consumed == ping.size());
}

int main()
{
    test_ping_command();
    test_set_command();
    test_incomplete_command();
    test_invalid_command();
    test_one_command_consumed_at_a_time();

    std::cout << "RESP parser tests passed\n";
    return 0;
}