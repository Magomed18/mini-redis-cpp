#include "command_parser.hpp"

#include <stdexcept>
#include <cassert>
#include <iostream>
#include <vector>

int main()
{
    Command command = parse_command("SET name \"John Smith\" EX 60");

    assert(command.operation == "SET");
    assert(command.key == "name");
    assert(command.value == "John Smith");
    assert(command.ttl == std::chrono::seconds{60});

    bool threw = false;

    try
    {
        parse_command("SET session abc PX 60");
    }
    catch (const std::invalid_argument&)
    {
        threw = true;
    }

    if (!threw)
    {
        std::cerr << "FAILED: invalid expiration option was accepted\n";
        return 1;
    }
    

    std::cout << "All command parser tests passed\n";


    const Command resp_set = parse_command(
    std::vector<std::string>{
        "SET", "name", "Alice Smith", "EX", "10"
    }
    );

    assert(resp_set.operation == "SET");
    assert(resp_set.key == "name");
    assert(resp_set.value == "Alice Smith");
    assert(resp_set.ttl.has_value());
    assert(resp_set.ttl.value() == std::chrono::seconds(10));

    return 0;
}