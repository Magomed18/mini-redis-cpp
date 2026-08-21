#pragma once

#include <string>

#include "command_parser.hpp"

class KeyValueStore;

// Describes everything the caller must do after executing a command.
struct CommandResult
{
    std::string response;
    bool storage_changed{false};
    bool close_connection{false};
};

// Executes one parsed command against the shared store.
[[nodiscard]] CommandResult execute_command(
    const Command& command,
    KeyValueStore& storage
);