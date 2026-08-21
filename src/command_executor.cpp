#include "command_executor.hpp"

#include "key_value_store.hpp"

CommandResult execute_command(
    const Command& command,
    KeyValueStore& storage
)
{
    if (command.operation.empty())
    {
        return {};
    }

    if (command.operation == "PING")
    {
        if (!command.key.empty() || !command.value.empty())
        {
            return {"Error: PING does not accept arguments\n"};
        }

        return {"PONG\n"};
    }

    if (command.operation == "SET")
    {
        if (command.key.empty() || command.value.empty())
        {
            return {"Error: SET requires key and value\n"};
        }

        if (command.ttl.has_value())
        {
            storage.set(
                command.key,
                command.value,
                command.ttl.value()
            );
        }
        else
        {
            storage.set(command.key, command.value);
        }

        // SET modified persistent application state.
        return {"OK\n", true};
    }

    if (command.operation == "GET")
    {
        if (command.key.empty())
        {
            return {"Error: GET requires key\n"};
        }

        if (!command.value.empty())
        {
            return {"Error: GET does not accept a value\n"};
        }

        const auto value = storage.get(command.key);

        if (!value.has_value())
        {
            return {"Error: Key not found\n"};
        }

        return {value.value() + "\n"};
    }

    if (command.operation == "DEL")
    {
        if (command.key.empty())
        {
            return {"Error: DEL requires key\n"};
        }

        if (!command.value.empty())
        {
            return {"Error: DEL does not accept a value\n"};
        }

        const bool removed = storage.del(command.key);

        if (!removed)
        {
            return {"Error: Key not found\n"};
        }

        return {"Key is removed\n", true};
    }

    if (command.operation == "EXISTS")
    {
        if (command.key.empty())
        {
            return {"Error: EXISTS requires key\n"};
        }

        if (!command.value.empty())
        {
            return {"Error: EXISTS does not accept a value\n"};
        }

        return {
            storage.exists(command.key) ? "1\n" : "0\n"
        };
    }

    if (command.operation == "HELP")
    {
        if (!command.key.empty() || !command.value.empty())
        {
            return {"Error: HELP does not accept arguments\n"};
        }

        return {
            "Commands:\n"
            "  PING\n"
            "  SET key value\n"
            "  GET key\n"
            "  DEL key\n"
            "  EXISTS key\n"
            "  HELP\n"
            "  EXIT\n"
        };
    }

    if (command.operation == "EXIT")
    {
        if (!command.key.empty() || !command.value.empty())
        {
            return {"Error: EXIT does not accept arguments\n"};
        }

        // EXIT closes this client connection, not the whole server.
        return {"BYE\n", false, true};
    }

    return {"Error: unknown command\n"};
}