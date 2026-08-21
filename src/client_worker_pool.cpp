#include "client_worker_pool.hpp"

#include <exception>
#include <iostream>
#include <optional>
#include <stdexcept>
#include <utility>

#include "client_session.hpp"

ClientWorkerPool::ClientWorkerPool(
    std::size_t worker_count,
    std::size_t max_pending_clients,
    KeyValueStore& storage,
    std::mutex& storage_mutex,
    std::string snapshot_file
)
    : storage_(storage),
      storage_mutex_(storage_mutex),
      snapshot_file_(std::move(snapshot_file)),
      max_pending_clients_(max_pending_clients)
{
    if (worker_count == 0)
    {
        throw std::invalid_argument(
            "Worker count must be greater than zero"
        );
    }

    if (max_pending_clients == 0)
    {
        throw std::invalid_argument(
            "Pending-client limit must be greater than zero"
        );
    }

    workers_.reserve(worker_count);

    try
    {
        for (std::size_t index = 0;
             index < worker_count;
             ++index)
        {
            workers_.emplace_back(
                &ClientWorkerPool::worker_loop,
                this
            );
        }
    }
    catch (...)
    {
        // Stop and join any threads that were created before failure.
        {
            std::lock_guard<std::mutex> lock(queue_mutex_);
            stopping_ = true;
        }

        client_available_.notify_all();

        for (std::thread& worker : workers_)
        {
            if (worker.joinable())
            {
                worker.join();
            }
        }

        throw;
    }
}

ClientWorkerPool::~ClientWorkerPool()
{
    {
        std::lock_guard<std::mutex> lock(queue_mutex_);
        stopping_ = true;
    }

    // Wake threads waiting for work or queue capacity.
    client_available_.notify_all();
    queue_space_available_.notify_all();

    for (std::thread& worker : workers_)
    {
        if (worker.joinable())
        {
            worker.join();
        }
    }
}

void ClientWorkerPool::submit(SocketHandle client_socket)
{
    std::unique_lock<std::mutex> lock(queue_mutex_);

    // Apply backpressure when the queue reaches its configured limit.
    queue_space_available_.wait(
        lock,
        [this]
        {
            return stopping_ ||
                   pending_clients_.size() <
                       max_pending_clients_;
        }
    );

    if (stopping_)
    {
        throw std::runtime_error(
            "Cannot submit client: worker pool is stopping"
        );
    }

    pending_clients_.push(std::move(client_socket));

    lock.unlock();

    // One waiting worker is enough to process the new client.
    client_available_.notify_one();
}

void ClientWorkerPool::worker_loop()
{
    while (true)
    {
        std::optional<SocketHandle> client_socket;

        {
            std::unique_lock<std::mutex> lock(queue_mutex_);

            client_available_.wait(
                lock,
                [this]
                {
                    return stopping_ ||
                           !pending_clients_.empty();
                }
            );

            if (stopping_)
            {
                return;
            }

            client_socket.emplace(
                std::move(pending_clients_.front())
            );

            pending_clients_.pop();
        }

        // A queue position became available.
        queue_space_available_.notify_one();

        try
        {
            handle_client(
                std::move(client_socket.value()),
                storage_,
                storage_mutex_,
                snapshot_file_
            );

            std::cout << "Client session ended.\n";
        }
        catch (const ClientConnectionError& error)
        {
            // A broken client must not terminate its worker.
            std::cerr << "Client connection error: "
                      << error.what() << '\n';
        }
        catch (const std::exception& error)
        {
            // Exceptions cannot escape a thread entry function:
            // doing so would call std::terminate().
            std::cerr << "Client session error: "
                      << error.what() << '\n';
        }
    }
}