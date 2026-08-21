#pragma once

#include <cstddef>
#include <string>
#include <string_view>
#include <vector>

enum class RespParseStatus
{
    Complete,
    Incomplete,
    Invalid
};

struct RespParseResult
{
    RespParseStatus status{RespParseStatus::Incomplete};
    std::vector<std::string> arguments;
    std::size_t bytes_consumed{0};
};

RespParseResult parse_resp_command(std::string_view data);