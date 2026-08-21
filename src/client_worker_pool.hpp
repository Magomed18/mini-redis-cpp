#pragma once

#include <condition_variable>
#include <cstddef>
#include <mutex>
#include <queue>
#include <string>
#include <thread>
#include <vector>

#include "socket_handle.hpp"

class KeyValueStore;

// Runs a fixed number of threads that process connected clients.
class ClientWorkerPool final
{
public:
    ClientWorkerPool(
        std::size_t worker_count,
        std::size_t max_pending_clients,
        KeyValueStore& storage,
        std::mutex& storage_mutex,
        std::string snapshot_file
    );

    ~ClientWorkerPool();

    ClientWorkerPool(const ClientWorkerPool&) = delete;
    ClientWorkerPool& operator=(const ClientWorkerPool&) = delete;
    ClientWorkerPool(ClientWorkerPool&&) = delete;
    ClientWorkerPool& operator=(ClientWorkerPool&&) = delete;

    // Transfers one connected socket into the bounded work queue.
    void submit(SocketHandle client_socket);

private:
    void worker_loop();

    KeyValueStore& storage_;
    std::mutex& storage_mutex_;
    std::string snapshot_file_;

    const std::size_t max_pending_clients_;

    std::queue<SocketHandle> pending_clients_;
    std::vector<std::thread> workers_;

    // Protects the queue itself, not the KeyValueStore.
    std::mutex queue_mutex_;

    // Wakes workers when a client becomes available.
    std::condition_variable client_available_;

    // Wakes submit() when queue capacity becomes available.
    std::condition_variable queue_space_available_;

    bool stopping_{false};
};