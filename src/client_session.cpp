#include "client_session.hpp"

#include <array>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <string>
#include <string_view>

#include "command_executor.hpp"
#include "command_parser.hpp"
#include "key_value_store.hpp"

namespace
{
    // Prevent an unfinished command from consuming unlimited memory.
    constexpr std::size_t max_command_length = 4096;

    // Send the entire response, handling partial send() operations.
    void send_all(SOCKET socket, std::string_view data)
    {
        if (data.size() >
            static_cast<std::size_t>(
                (std::numeric_limits<int>::max)()))
        {
            throw std::runtime_error("Response is too large to send");
        }

        std::size_t total_sent = 0;

        while (total_sent < data.size())
        {
            const int bytes_sent = ::send(
                socket,
                data.data() + total_sent,
                static_cast<int>(data.size() - total_sent),
                0
            );

            if (bytes_sent == SOCKET_ERROR)
            {
                const int error_code = WSAGetLastError();

                throw  ClientConnectionError(
                    "Failed to send response. Winsock error: "
                    + std::to_string(error_code)
                );
            }

            if (bytes_sent == 0)
            {
                throw ClientConnectionError(
                    "Connection closed while sending response"
                );
            }

            total_sent +=
                static_cast<std::size_t>(bytes_sent);
        }
    }
}



void handle_client(
    SocketHandle client_socket,
    KeyValueStore& storage,
    std::mutex& storage_mutex,
    const std::string& snapshot_file
)
{

    std::array<char, 1024> receive_buffer{};
    std::string pending_data;
    bool close_client = false;
    // Release a worker when a client sends nothing for 60 seconds.
    const DWORD receive_timeout_ms = 60'000;

    if (::setsockopt(
            client_socket.get(),
            SOL_SOCKET,
            SO_RCVTIMEO,
            reinterpret_cast<const char*>(&receive_timeout_ms),
            sizeof(receive_timeout_ms)) == SOCKET_ERROR)
    {
        const int error_code = WSAGetLastError();

        throw ClientConnectionError(
            "Failed to set client receive timeout. Winsock error: "
            + std::to_string(error_code)
        );
    }

    while (true)
    {
        const int bytes_received = ::recv(
            client_socket.get(),
            receive_buffer.data(),
            static_cast<int>(receive_buffer.size()),
            0
        );

        if (bytes_received == SOCKET_ERROR)
        {
            const int error_code = WSAGetLastError();

            if (error_code == WSAECONNRESET)
            {
                std::cout
                    << "Client reset the connection.\n";
                break;
            }

            if (error_code == WSAETIMEDOUT)
            {
                std::cout << "Client disconnected due to inactivity.\n";
                break;
            }

            throw ClientConnectionError(
                "Failed to receive data. Winsock error: "
                + std::to_string(error_code)
            );
        }

        if (bytes_received == 0)
        {
            std::cout << "Client disconnected.\n";
            break;
        }

        pending_data.append(
            receive_buffer.data(),
            static_cast<std::size_t>(bytes_received)
        );

        std::size_t newline_position = 0;

        // Extract every complete newline-terminated command.
        while ((newline_position = pending_data.find('\n'))
               != std::string::npos)
        {
            if (newline_position > max_command_length)
            {
                send_all(
                    client_socket.get(),
                    "Error: command is too long\n"
                );

                close_client = true;
                break;
            }

            std::string received_command =
                pending_data.substr(0, newline_position);

            pending_data.erase(0, newline_position + 1);

            // Accept both "\n" and Windows-style "\r\n".
            if (!received_command.empty() &&
                received_command.back() == '\r')
            {
                received_command.pop_back();
            }

            std::cout << "Complete command: "
                      << received_command << '\n';

            const Command parsed =
                parse_command(received_command);

            CommandResult result{};

            {
                // Only one client may execute a storage command and save
                // the snapshot at a time.
                std::lock_guard<std::mutex> lock(storage_mutex);

                result = execute_command(parsed, storage);

                if (result.storage_changed)
                {
                    storage.save_to_file(snapshot_file);
                }
            }

            // Network sending happens after releasing the storage lock.
            if (!result.response.empty())
            {
                send_all(client_socket.get(), result.response);
            }

            if (!result.response.empty())
            {
                send_all(
                    client_socket.get(),
                    result.response
                );
            }

            if (result.close_connection)
            {
                close_client = true;
                break;
            }
        }

        // No newline exists, but the unfinished command is too large.
        if (!close_client &&
            pending_data.size() > max_command_length)
        {
            send_all(
                client_socket.get(),
                "Error: command is too long\n"
            );

            close_client = true;
        }

        if (close_client)
        {
            break;
        }
    }

    // client_socket is destroyed here and closes automatically.
}