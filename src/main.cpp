#include <iostream>
#include <string>
#include <ws2tcpip.h>
#include <stdexcept>
#include <mutex>

#include "key_value_store.hpp"
#include "winsock_runtime.hpp"
#include "socket_handle.hpp"
#include "client_worker_pool.hpp"


int main()
{
    try
    {
        
    WinsockRuntime winsock{};

    KeyValueStore storage;
    std::mutex storage_mutex;

    const std::string snapshot_file = "snapshot.txt";


    try
    {
        storage.load_from_file(snapshot_file);
    }
    catch (const std::exception& error)
    {
        std::cerr << "Failed to load snapshot: "
                  << error.what() << '\n';
        return 1;
    }

    SocketHandle server_socket{
        ::socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)
    };

    if (!server_socket.valid())
    {
        const int error_code = WSAGetLastError();

        throw std::runtime_error(
            "Failed to create TCP socket. Winsock error: "
            + std::to_string(error_code)
        );
    }

    sockaddr_in server_address{};
    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(6379);

    if (::inet_pton(
            AF_INET,
            "127.0.0.1",
            &server_address.sin_addr) != 1)
    {
        throw std::runtime_error("Invalid server IP address");
    }

    if (::bind(
            server_socket.get(),
            reinterpret_cast<const sockaddr*>(&server_address),
            sizeof(server_address)) == SOCKET_ERROR)
    {
        const int error_code = WSAGetLastError();

        throw std::runtime_error(
            "Failed to bind TCP socket. Winsock error: "
            + std::to_string(error_code)
        );
    }

    // Turn the bound socket into a listening server socket.
    // SOMAXCONN lets Winsock choose a reasonable connection queue size.
    if (::listen(server_socket.get(), SOMAXCONN) == SOCKET_ERROR)
    {
        // Capture the error immediately before another Winsock call changes it.
        const int error_code = WSAGetLastError();

        throw std::runtime_error(
            "Failed to listen on TCP socket. Winsock error: "
            + std::to_string(error_code)
        );
    }

    // Four workers can serve four connected clients concurrently.
    // Additional accepted clients wait in a bounded queue.
    constexpr std::size_t worker_count = 4;
    constexpr std::size_t max_pending_clients = 32;

    ClientWorkerPool worker_pool{
        worker_count,
        max_pending_clients,
        storage,
        storage_mutex,
        snapshot_file
    };

    std::cout << "Mini Redis listening on 127.0.0.1:6379\n";

    while (true)
    {
        std::cout << "Waiting for a client...\n";


        // accept() writes the connecting client's address here.
        sockaddr_in client_address{};
        int client_address_size =
            static_cast<int>(sizeof(client_address));

        SocketHandle client_socket{
            ::accept(
                server_socket.get(),
                reinterpret_cast<sockaddr*>(&client_address),
                &client_address_size
            )
        };

        if (!client_socket.valid())
        {
            const int error_code = WSAGetLastError();

            throw std::runtime_error(
                "Failed to accept client. Winsock error: "
                + std::to_string(error_code)
            );
        }

        std::cout << "Client connected.\n";

        // Transfer the connection to a worker and immediately
        // return to accept() for another client.
        worker_pool.submit(std::move(client_socket));

            
    }
    
    }

    catch (const std::exception& error)
    {
        std::cerr << "Fatal error: "
                  << error.what() << '\n';
        return 1;
    }

    return 0;

}