#include <cassert>
#include <exception>
#include <filesystem>
#include <mutex>
#include <stdexcept>
#include <string>
#include <string_view>
#include <system_error>
#include <thread>
#include <utility>

#include <ws2tcpip.h>

#include "client_session.hpp"
#include "key_value_store.hpp"
#include "socket_handle.hpp"
#include "winsock_runtime.hpp"

namespace
{

void send_test_data(SOCKET socket, std::string_view data)
{
    std::size_t total_sent = 0;

    while (total_sent < data.size())
    {
        const int bytes_sent = ::send(
            socket,
            data.data() + total_sent,
            static_cast<int>(data.size() - total_sent),
            0
        );

        if (bytes_sent == SOCKET_ERROR || bytes_sent == 0)
        {
            throw std::runtime_error(
                "Integration test failed to send data"
            );
        }

        total_sent +=
            static_cast<std::size_t>(bytes_sent);
    }
}

std::string receive_exact(
    SOCKET socket,
    std::size_t expected_size)
{
    std::string result(expected_size, '\0');
    std::size_t total_received = 0;

    while (total_received < expected_size)
    {
        const int bytes_received = ::recv(
            socket,
            result.data() + total_received,
            static_cast<int>(
                expected_size - total_received
            ),
            0
        );

        if (bytes_received == SOCKET_ERROR ||
            bytes_received == 0)
        {
            throw std::runtime_error(
                "Integration test failed to receive data"
            );
        }

        total_received +=
            static_cast<std::size_t>(bytes_received);
    }

    return result;
}

} // namespace

int main()
{
    WinsockRuntime winsock;

    SocketHandle listener{
        ::socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)
    };

    assert(listener.valid());

    sockaddr_in address{};
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = htonl(INADDR_LOOPBACK);

    // Port zero asks Windows to choose an unused port.
    address.sin_port = 0;

    assert(
        ::bind(
            listener.get(),
            reinterpret_cast<const sockaddr*>(&address),
            sizeof(address)
        ) != SOCKET_ERROR
    );

    assert(
        ::listen(listener.get(), 1) != SOCKET_ERROR
    );

    int address_size = sizeof(address);

    assert(
        ::getsockname(
            listener.get(),
            reinterpret_cast<sockaddr*>(&address),
            &address_size
        ) != SOCKET_ERROR
    );

    SocketHandle client_socket{
        ::socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)
    };

    assert(client_socket.valid());

    const DWORD receive_timeout_ms = 5000;

    assert(
        ::setsockopt(
            client_socket.get(),
            SOL_SOCKET,
            SO_RCVTIMEO,
            reinterpret_cast<const char*>(
                &receive_timeout_ms
            ),
            sizeof(receive_timeout_ms)
        ) != SOCKET_ERROR
    );

    assert(
        ::connect(
            client_socket.get(),
            reinterpret_cast<const sockaddr*>(&address),
            sizeof(address)
        ) != SOCKET_ERROR
    );

    SocketHandle server_client{
        ::accept(listener.get(), nullptr, nullptr)
    };

    assert(server_client.valid());

    KeyValueStore storage;
    std::mutex storage_mutex;

    const std::string snapshot_file =
        "client_session_test_snapshot.txt";

    std::error_code ignored_error;
    std::filesystem::remove(
        snapshot_file,
        ignored_error
    );

    std::exception_ptr session_error;

    std::thread session_thread(
        [
            socket = std::move(server_client),
            &storage,
            &storage_mutex,
            &snapshot_file,
            &session_error
        ]() mutable
        {
            try
            {
                handle_client(
                    std::move(socket),
                    storage,
                    storage_mutex,
                    snapshot_file
                );
            }
            catch (...)
            {
                session_error =
                    std::current_exception();
            }
        }
    );

    std::string ping_response;
    std::string pipeline_response;
    std::string exit_response;

    try
    {
        // Lowercase command also verifies normalization.
        const std::string ping_request =
            "*1\r\n$4\r\nping\r\n";

        send_test_data(
            client_socket.get(),
            ping_request
        );

        ping_response = receive_exact(
            client_socket.get(),
            std::string("+PONG\r\n").size()
        );

        const std::string set_request =
            "*3\r\n"
            "$3\r\nSET\r\n"
            "$4\r\nname\r\n"
            "$11\r\nAlice Smith\r\n";

        const std::string get_request =
            "*2\r\n"
            "$3\r\nGET\r\n"
            "$4\r\nname\r\n";

        // Send two commands together to test pipelining.
        send_test_data(
            client_socket.get(),
            set_request + get_request
        );

        const std::string expected_pipeline =
            "+OK\r\n"
            "$11\r\nAlice Smith\r\n";

        pipeline_response = receive_exact(
            client_socket.get(),
            expected_pipeline.size()
        );

        const std::string exit_request =
            "*1\r\n$4\r\nEXIT\r\n";

        send_test_data(
            client_socket.get(),
            exit_request
        );

        exit_response = receive_exact(
            client_socket.get(),
            std::string("+BYE\r\n").size()
        );

        session_thread.join();
    }
    catch (...)
    {
        ::shutdown(client_socket.get(), SD_BOTH);

        if (session_thread.joinable())
        {
            session_thread.join();
        }

        std::filesystem::remove(
            snapshot_file,
            ignored_error
        );

        throw;
    }

    if (session_error)
    {
        std::rethrow_exception(session_error);
    }

    std::filesystem::remove(
        snapshot_file,
        ignored_error
    );

    assert(ping_response == "+PONG\r\n");

    assert(
        pipeline_response ==
        "+OK\r\n$11\r\nAlice Smith\r\n"
    );

    assert(exit_response == "+BYE\r\n");

    return 0;
}