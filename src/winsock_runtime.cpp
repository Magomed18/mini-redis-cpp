#include "winsock_runtime.hpp"

// winsock2.h must be included before any header that might include windows.h.
#include <winsock2.h>

#include <stdexcept>
#include <string>


 
WinsockRuntime::WinsockRuntime()
{
    WSADATA socket_data{};

    // Request Winsock version 2.2.
    const int result =
        WSAStartup(MAKEWORD(2, 2), &socket_data);

    if (result != 0)
    {
        throw std::runtime_error(
            "WSAStartup failed with error code "
            + std::to_string(result)
        );
    }
}

WinsockRuntime::~WinsockRuntime() noexcept
{
    // Release the Winsock resources initialized by WSAStartup().
    WSACleanup();
}