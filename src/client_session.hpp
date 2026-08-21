#pragma once

#include <string>
#include <stdexcept>
#include <mutex>

#include "socket_handle.hpp"

class KeyValueStore;

// Represents a failure limited to one client connection.
class ClientConnectionError : public std::runtime_error
{
public:
    using std::runtime_error::runtime_error;
};

// Owns and processes one connected client until it disconnects or sends EXIT.
void handle_client(
    SocketHandle client_socket,
    KeyValueStore& storage,
    std::mutex& storage_mutex,
    const std::string& snapshot_file
);