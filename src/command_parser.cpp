#include "command_parser.hpp"

#include <iomanip>
#include <sstream>
#include <stdexcept>
#include <vector>

namespace
{

std::chrono::seconds parse_ttl(const std::string& text)
{
    std::size_t converted_characters = 0;
    int ttl_number = 0;

    try
    {
        ttl_number = std::stoi(
            text,
            &converted_characters
        );
    }
    catch (const std::exception&)
    {
        throw std::invalid_argument(
            "TTL must be a positive integer"
        );
    }

    if (converted_characters != text.size() ||
        ttl_number <= 0)
    {
        throw std::invalid_argument(
            "TTL must be a positive integer"
        );
    }

    return std::chrono::seconds(ttl_number);
}

} // namespace

Command parse_command(
    const std::vector<std::string>& arguments)
{
    if (arguments.empty())
    {
        return {};
    }

    Command command;
    command.operation = arguments[0];

    if (arguments.size() >= 2)
    {
        command.key = arguments[1];
    }

    if (command.operation == "SET")
    {
        if (arguments.size() < 3)
        {
            throw std::invalid_argument(
                "SET requires a value"
            );
        }

        command.value = arguments[2];

        if (arguments.size() == 3)
        {
            return command;
        }

        if (arguments[3] != "EX")
        {
            throw std::invalid_argument(
                "SET requires expiration token EX"
            );
        }

        if (arguments.size() == 4)
        {
            throw std::invalid_argument(
                "EX requires a TTL"
            );
        }

        if (arguments.size() > 5)
        {
            throw std::invalid_argument(
                "Too many arguments for SET"
            );
        }

        command.ttl = parse_ttl(arguments[4]);
        return command;
    }

    // Preserve an unwanted extra argument so the executor
    // can produce its existing validation response.
    if (arguments.size() >= 3)
    {
        command.value = arguments[2];
    }

    if (arguments.size() > 3)
    {
        throw std::invalid_argument(
            "Too many command arguments"
        );
    }

    return command;
}

Command parse_command(const std::string& command_text)
{
    std::istringstream input(command_text);
    std::vector<std::string> arguments;
    std::string argument;

    while (input >> std::quoted(argument))
    {
        arguments.push_back(argument);
    }

    return parse_command(arguments);
}