#pragma once

#include <chrono>
#include <optional>
#include <string>
#include <vector>



struct Command
{
    std::string operation;
    std::string key;
    std::string value;
    std::optional<std::chrono::seconds> ttl;
};

Command parse_command(const std::string& command);

Command parse_command(
    const std::vector<std::string>& arguments
);