#include "command_parser.hpp"

#include <stdexcept>
#include <cassert>
#include <iostream>

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

    return 0;
}