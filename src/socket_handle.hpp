#pragma once

#include <winsock2.h>

class SocketHandle final
{
public:
    explicit SocketHandle(
        SOCKET handle = INVALID_SOCKET
    ) noexcept;

    ~SocketHandle() noexcept;

    SocketHandle(const SocketHandle&) = delete;
    SocketHandle& operator=(const SocketHandle&) = delete;

    SocketHandle(SocketHandle&& other) noexcept;
    SocketHandle& operator=(SocketHandle&& other) noexcept;

    [[nodiscard]] SOCKET get() const noexcept;
    [[nodiscard]] bool valid() const noexcept;

private:
    SOCKET handle_;
};