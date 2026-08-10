#pragma once

#include <chrono>
#include <optional>
#include <string>
#include <unordered_map>

class KeyValueStore
{
public:

    // Operation methods
    void set(const std::string& key, const std::string& value);
    void set
    (
    const std::string& key,
    const std::string& value,
    std::chrono::seconds ttl
    );
    std::optional<std::string> get(const std::string& key) const;
    bool del(const std::string& key);
    bool exists(const std::string& key) const;

    // Disk IO
    void save_to_file(const std::string& filename) const;
    void load_from_file(const std::string& filename);

private:
    struct Entry
    {
        std::string value;
        std::optional<std::chrono::steady_clock::time_point> expires_at;
    };
    
    std::unordered_map<std::string, Entry> data_;
};