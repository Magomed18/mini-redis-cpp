#include <cassert>

#include "command_executor.hpp"
#include "key_value_store.hpp"

int main()
{
    KeyValueStore storage;

    // PING should produce a health-check response.
    {
        Command command{};
        command.operation = "PING";

        const CommandResult result =
            execute_command(command, storage);

        assert(result.response == "PONG\n");
        assert(!result.storage_changed);
        assert(!result.close_connection);
    }

    // SET should modify storage and request persistence.
    {
        Command command{};
        command.operation = "SET";
        command.key = "name";
        command.value = "Alice";

        const CommandResult result =
            execute_command(command, storage);

        assert(result.response == "OK\n");
        assert(result.storage_changed);
        assert(!result.close_connection);

        const auto stored_value = storage.get("name");

        assert(stored_value.has_value());
        assert(stored_value.value() == "Alice");
    }

    // GET should return the stored value without modifying storage.
    {
        Command command{};
        command.operation = "GET";
        command.key = "name";

        const CommandResult result =
            execute_command(command, storage);

        assert(result.response == "Alice\n");
        assert(!result.storage_changed);
        assert(!result.close_connection);
    }

    // EXIT should close only the current client connection.
    {
        Command command{};
        command.operation = "EXIT";

        const CommandResult result =
            execute_command(command, storage);

        assert(result.response == "BYE\n");
        assert(!result.storage_changed);
        assert(result.close_connection);
    }

    // EXISTS should report that the stored key exists.
    {
        Command command{};
        command.operation = "EXISTS";
        command.key = "name";

        const CommandResult result =
            execute_command(command, storage);

        assert(result.response == "1\n");
        assert(!result.storage_changed);
    }

    // DEL should remove the key and request persistence.
    {
        Command command{};
        command.operation = "DEL";
        command.key = "name";

        const CommandResult result =
            execute_command(command, storage);

        assert(result.response == "Key is removed\n");
        assert(result.storage_changed);
        assert(!storage.exists("name"));
    }

    // GET should report a missing key after deletion.
    {
        Command command{};
        command.operation = "GET";
        command.key = "name";

        const CommandResult result =
            execute_command(command, storage);

        assert(result.response == "Error: Key not found\n");
        assert(!result.storage_changed);
    }

    // Invalid SET must not modify storage.
    {
        Command command{};
        command.operation = "SET";
        command.key = "incomplete";

        const CommandResult result =
            execute_command(command, storage);

        assert(result.response ==
            "Error: SET requires key and value\n");
        assert(!result.storage_changed);
        assert(!storage.exists("incomplete"));
    }

    // Unknown commands should produce a clear error.
    {
        Command command{};
        command.operation = "UNKNOWN";

        const CommandResult result =
            execute_command(command, storage);

        assert(result.response == "Error: unknown command\n");
        assert(!result.storage_changed);
        assert(!result.close_connection);
    }

    return 0;
}