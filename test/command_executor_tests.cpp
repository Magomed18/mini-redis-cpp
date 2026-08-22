#include <cassert>

#include "command_executor.hpp"
#include "key_value_store.hpp"

int main()
{
    KeyValueStore storage;

    // PING
    {
        Command command{};
        command.operation = "PING";

        const CommandResult result =
            execute_command(command, storage);

        assert(result.response == "+PONG\r\n");
        assert(!result.storage_changed);
        assert(!result.close_connection);
    }

    // SET
    {
        Command command{};
        command.operation = "SET";
        command.key = "name";
        command.value = "Alice";

        const CommandResult result =
            execute_command(command, storage);

        assert(result.response == "+OK\r\n");
        assert(result.storage_changed);
        assert(!result.close_connection);

        const auto stored_value = storage.get("name");

        assert(stored_value.has_value());
        assert(stored_value.value() == "Alice");
    }

    // GET existing key
    {
        Command command{};
        command.operation = "GET";
        command.key = "name";

        const CommandResult result =
            execute_command(command, storage);

        assert(result.response == "$5\r\nAlice\r\n");
        assert(!result.storage_changed);
        assert(!result.close_connection);
    }

    // EXIT
    {
        Command command{};
        command.operation = "EXIT";

        const CommandResult result =
            execute_command(command, storage);

        assert(result.response == "+BYE\r\n");
        assert(!result.storage_changed);
        assert(result.close_connection);
    }

    // EXISTS
    {
        Command command{};
        command.operation = "EXISTS";
        command.key = "name";

        const CommandResult result =
            execute_command(command, storage);

        assert(result.response == ":1\r\n");
        assert(!result.storage_changed);
    }

    // Successful DEL
    {
        Command command{};
        command.operation = "DEL";
        command.key = "name";

        const CommandResult result =
            execute_command(command, storage);

        assert(result.response == ":1\r\n");
        assert(result.storage_changed);
        assert(!storage.exists("name"));
    }

    // GET missing key
    {
        Command command{};
        command.operation = "GET";
        command.key = "name";

        const CommandResult result =
            execute_command(command, storage);

        assert(result.response == "$-1\r\n");
        assert(!result.storage_changed);
    }

    // Invalid SET
    {
        Command command{};
        command.operation = "SET";
        command.key = "incomplete";

        const CommandResult result =
            execute_command(command, storage);

        assert(
            result.response ==
            "-ERR SET requires key and value\r\n"
        );

        assert(!result.storage_changed);
        assert(!storage.exists("incomplete"));
    }

    // Unknown command
    {
        Command command{};
        command.operation = "UNKNOWN";

        const CommandResult result =
            execute_command(command, storage);

        assert(
            result.response ==
            "-ERR unknown command\r\n"
        );

        assert(!result.storage_changed);
        assert(!result.close_connection);
    }

    return 0;
}