#include "key_value_store.hpp"
#include <fstream>
#include <stdexcept>
#include <iomanip>
#include <filesystem>

// Permanent entry set
void KeyValueStore::set(
    const std::string& key,
    const std::string& value
)
{
    data_[key] = Entry{value, std::nullopt};
}

// Entry with TTL
void KeyValueStore::set(
    const std::string& key,
    const std::string& value,
    std::chrono::seconds ttl
)
{
    const auto expiration_time =
        std::chrono::steady_clock::now() + ttl;

    data_[key] = Entry{value, expiration_time};
}

std::optional<std::string> KeyValueStore::get(const std::string& key) const
{
    auto it = data_.find(key);

    if (it == data_.end())
    {
        return std::nullopt;
    }

    if (it->second.expires_at.has_value() &&
        std::chrono::steady_clock::now() >= it->second.expires_at.value())
    {
        return std::nullopt;
    }

    return it->second.value;
}

bool KeyValueStore::del(const std::string& key)
{
    const auto it = data_.find(key);

    if (it == data_.end())
    {
        return false;
    }

    if (it->second.expires_at.has_value() &&
        std::chrono::steady_clock::now() >=
            it->second.expires_at.value())
    {
        data_.erase(it);
        return false;
    }

    data_.erase(it);
    return true;
}

bool KeyValueStore::exists(const std::string& key) const
{
    return get(key).has_value();
}

void KeyValueStore::save_to_file(const std::string& filename) const
{
    std::ofstream output(filename);

    if (!output)
    {
        throw std::runtime_error("Could not open snapshot file");
    }

    output << "MINI_REDIS_V1\n";

    const auto steady_now = std::chrono::steady_clock::now();
    const auto system_now = std::chrono::system_clock::now();

    for (const auto& [key, entry] : data_)
    {
        long long expiration_timestamp = -1;

        if (entry.expires_at.has_value())
        {
            const auto remaining =
                entry.expires_at.value() - steady_now;

            if (remaining <= std::chrono::steady_clock::duration::zero())
            {
                continue;
            }

            const auto system_expiration = system_now + remaining;

            expiration_timestamp =
                std::chrono::ceil<std::chrono::seconds>(
                    system_expiration.time_since_epoch()
                ).count();
        }

        output
            << std::quoted(key) << ' '
            << std::quoted(entry.value) << ' '
            << expiration_timestamp << '\n';
    }

    if (!output)
    {
        throw std::runtime_error("Could not write snapshot file");
    }
}

void KeyValueStore::load_from_file(const std::string& filename)
{
    if (!std::filesystem::exists(filename))
    {
        return;
    }

    std::ifstream input(filename);

    if (!input)
    {
        throw std::runtime_error("Could not open snapshot file");
    }

    std::string header;

    if (!std::getline(input, header))
    {
        throw std::runtime_error("Snapshot file is empty");
    }

    if (header != "MINI_REDIS_V1")
    {
        throw std::runtime_error("Unsupported snapshot format");
    }

    std::string line;
std::size_t line_number = 1;

while (std::getline(input, line))
{
    ++line_number;

    if (line.empty())
    {
        continue;
    }

    std::istringstream line_input(line);

    std::string key;
    std::string value;
    long long expiration_timestamp;

    if (!(line_input
          >> std::quoted(key)
          >> std::quoted(value)
          >> expiration_timestamp))
    {
        throw std::runtime_error(
            "Invalid snapshot entry on line " +
            std::to_string(line_number)
        );
    }

    std::string extra;

    if (line_input >> extra)
    {
        throw std::runtime_error(
            "Extra data on snapshot line " +
            std::to_string(line_number)
        );
    }

    if (expiration_timestamp == -1)
    {
        set(key, value);
        continue;
    }

    if (expiration_timestamp < 0)
    {
        throw std::runtime_error(
            "Invalid expiration timestamp on line " +
            std::to_string(line_number)
        );
    }

    const auto expiration_time =
        std::chrono::system_clock::time_point{
            std::chrono::seconds{expiration_timestamp}
        };

    const auto now = std::chrono::system_clock::now();

    if (expiration_time <= now)
    {
        continue;
    }

    const auto remaining =
        std::chrono::ceil<std::chrono::seconds>(
            expiration_time - now
        );

    set(key, value, remaining);
}
}