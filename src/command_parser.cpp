#include "command_parser.hpp"
#include <iomanip>
#include <sstream>

Command parse_command(const std::string& command)
{
    std::istringstream input(command);

    std::string operation;
    std::string key;
    std::string value;
    std::string expiration_option;
    std::optional<std::chrono::seconds> ttl;
    std::string ttl_text;
    std::string extra_token;

    


    input >> operation >> key;

    if (!(input >> std::quoted(value)) && operation == "SET")
    {
        throw std::invalid_argument("SET requires a value");
        

    }

    
    if (input >> expiration_option)
    {
        // expiration_option should be "EX"
        if (expiration_option != "EX")
        {
            throw std::invalid_argument("SET requires expration token");
        }

        if (!(input >> ttl_text))
        {
            throw std::invalid_argument("EX requires a TTL");
        }

        std::size_t converted_characters = 0;
        int ttl_num = 0;

        try
        {
            ttl_num = std::stoi(ttl_text, &converted_characters);
            
        }
        catch (const std::exception&)
        {
            throw std::invalid_argument("TTL must be a positive integer");
        }

        if (converted_characters != ttl_text.size() || ttl_num <= 0)
        {
            throw std::invalid_argument("TTL must be a positive integer");
        }

        ttl = std::chrono::seconds(ttl_num);
    }

    

    if (input >> extra_token)
    {
        throw std::invalid_argument("Too many arguments for SET");
    }



    return {operation, key, value, ttl};
}
