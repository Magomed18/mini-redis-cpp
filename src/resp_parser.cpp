#include "resp_parser.hpp"

#include <charconv>
#include <system_error>
#include <utility>

namespace
{

constexpr std::size_t max_argument_count = 16;
constexpr std::size_t max_argument_length = 4096;

bool parse_size(std::string_view text, std::size_t& value)
{
    if (text.empty())
    {
        return false;
    }

    const char* begin = text.data();
    const char* end = begin + text.size();

    const auto conversion = std::from_chars(begin, end, value);

    return conversion.ec == std::errc{} &&
           conversion.ptr == end;
}

RespParseResult invalid_result()
{
    RespParseResult result;
    result.status = RespParseStatus::Invalid;
    return result;
}

} // namespace

RespParseResult parse_resp_command(std::string_view data)
{
    if (data.empty())
    {
        return {};
    }

    // RESP commands must begin with an array marker.
    if (data[0] != '*')
    {
        return invalid_result();
    }

    const std::size_t array_line_end = data.find("\r\n");

    if (array_line_end == std::string_view::npos)
    {
        return {};
    }

    std::size_t argument_count = 0;

    if (!parse_size(
            data.substr(1, array_line_end - 1),
            argument_count) ||
        argument_count == 0 ||
        argument_count > max_argument_count)
    {
        return invalid_result();
    }

    std::size_t position = array_line_end + 2;
    std::vector<std::string> arguments;
    arguments.reserve(argument_count);

    for (std::size_t index = 0;
         index < argument_count;
         ++index)
    {
        if (position >= data.size())
        {
            return {};
        }

        if (data[position] != '$')
        {
            return invalid_result();
        }

        const std::size_t length_line_end =
            data.find("\r\n", position);

        if (length_line_end == std::string_view::npos)
        {
            return {};
        }

        std::size_t argument_length = 0;

        if (!parse_size(
                data.substr(
                    position + 1,
                    length_line_end - position - 1),
                argument_length) ||
            argument_length > max_argument_length)
        {
            return invalid_result();
        }

        position = length_line_end + 2;

        const std::size_t required_size =
            position + argument_length + 2;

        if (data.size() < required_size)
        {
            return {};
        }

        const std::size_t terminator_position =
            position + argument_length;

        if (data[terminator_position] != '\r' ||
            data[terminator_position + 1] != '\n')
        {
            return invalid_result();
        }

        arguments.emplace_back(
            data.data() + position,
            argument_length
        );

        position = terminator_position + 2;
    }

    RespParseResult result;
    result.status = RespParseStatus::Complete;
    result.arguments = std::move(arguments);
    result.bytes_consumed = position;

    return result;
}