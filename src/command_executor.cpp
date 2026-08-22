#include "command_executor.hpp"

#include "key_value_store.hpp"
#include "resp_encoder.hpp"

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
            return {
                encode_resp_error(
                    "PING does not accept arguments"
                )
            };
        }

        return {encode_resp_simple_string("PONG")};
    }

    if (command.operation == "SET")
    {
        if (command.key.empty() || command.value.empty())
        {
            return {
                encode_resp_error(
                    "SET requires key and value"
                )
            };
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

        return {
            encode_resp_simple_string("OK"),
            true
        };
    }

    if (command.operation == "GET")
    {
        if (command.key.empty())
        {
            return {
                encode_resp_error("GET requires key")
            };
        }

        if (!command.value.empty())
        {
            return {
                encode_resp_error(
                    "GET does not accept a value"
                )
            };
        }

        const auto value = storage.get(command.key);

        if (!value.has_value())
        {
            return {encode_resp_null_bulk_string()};
        }

        return {
            encode_resp_bulk_string(value.value())
        };
    }

    if (command.operation == "DEL")
    {
        if (command.key.empty())
        {
            return {
                encode_resp_error("DEL requires key")
            };
        }

        if (!command.value.empty())
        {
            return {
                encode_resp_error(
                    "DEL does not accept a value"
                )
            };
        }

        const bool removed = storage.del(command.key);

        return {
            encode_resp_integer(removed ? 1 : 0),
            removed
        };
    }

    if (command.operation == "EXISTS")
    {
        if (command.key.empty())
        {
            return {
                encode_resp_error("EXISTS requires key")
            };
        }

        if (!command.value.empty())
        {
            return {
                encode_resp_error(
                    "EXISTS does not accept a value"
                )
            };
        }

        return {
            encode_resp_integer(
                storage.exists(command.key) ? 1 : 0
            )
        };
    }

    if (command.operation == "HELP")
    {
        if (!command.key.empty() || !command.value.empty())
        {
            return {
                encode_resp_error(
                    "HELP does not accept arguments"
                )
            };
        }

        return {
            encode_resp_bulk_string(
                "Commands:\n"
                "  PING\n"
                "  SET key value [EX seconds]\n"
                "  GET key\n"
                "  DEL key\n"
                "  EXISTS key\n"
                "  HELP\n"
                "  EXIT\n"
            )
        };
    }

    if (command.operation == "EXIT")
    {
        if (!command.key.empty() || !command.value.empty())
        {
            return {
                encode_resp_error(
                    "EXIT does not accept arguments"
                )
            };
        }

        return {
            encode_resp_simple_string("BYE"),
            false,
            true
        };
    }

    return {
        encode_resp_error("unknown command")
    };
}