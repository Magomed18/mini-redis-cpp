#include "socket_handle.hpp"

#include <utility>

SocketHandle::SocketHandle(SOCKET handle) noexcept
    : handle_(handle)
{
}

SocketHandle::~SocketHandle() noexcept
{
    if (valid())
    {
        closesocket(handle_);
    }
}

SocketHandle::SocketHandle(SocketHandle&& other) noexcept
    : handle_(std::exchange(other.handle_, INVALID_SOCKET))
{
}

SocketHandle& SocketHandle::operator=(SocketHandle&& other) noexcept
{
    if (this != &other)
    {
        if (valid())
        {
            closesocket(handle_);
        }

        handle_ =
            std::exchange(other.handle_, INVALID_SOCKET);
    }

    return *this;
}

SOCKET SocketHandle::get() const noexcept
{
    return handle_;
}

bool SocketHandle::valid() const noexcept
{
    return handle_ != INVALID_SOCKET;
}