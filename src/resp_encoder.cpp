#include "resp_encoder.hpp"

#include <string>

std::string encode_resp_simple_string(std::string_view value)
{
    return "+" + std::string(value) + "\r\n";
}

std::string encode_resp_error(std::string_view message)
{
    return "-ERR " + std::string(message) + "\r\n";
}

std::string encode_resp_bulk_string(std::string_view value)
{
    return "$" + std::to_string(value.size()) +
           "\r\n" +
           std::string(value) +
           "\r\n";
}

std::string encode_resp_integer(long long value)
{
    return ":" + std::to_string(value) + "\r\n";
}

std::string encode_resp_null_bulk_string()
{
    return "$-1\r\n";
}