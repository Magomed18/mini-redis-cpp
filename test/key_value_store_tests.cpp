#include "key_value_store.hpp"

#include <chrono>
#include <cassert>
#include <iostream>
#include <filesystem>
#include <fstream>

int main()
{
    KeyValueStore storage;

    // A new store must not contain this key.
    assert(!storage.exists("name"));
    assert(!storage.get("name").has_value());

    // SET + GET
    storage.set("name", "Alice");

    auto value = storage.get("name");

    assert(value.has_value());
    assert(value.value() == "Alice");
    assert(storage.exists("name"));

    // A key without TTL must remain available.
    storage.set("permanent", "value");

    assert(storage.exists("permanent"));
    assert(storage.get("permanent").has_value());

  

    // A key with a zero-second TTL must be immediately expired.
    storage.set(
        "expired",
        "old value",
        std::chrono::seconds{0}
    );

    assert(!storage.get("expired").has_value());
    assert(!storage.exists("expired"));
    assert(!storage.del("expired"));

    // SET overwrites an existing value.
    storage.set("name", "Bob");

    value = storage.get("name");

    assert(value.has_value());
    assert(value.value() == "Bob");

    // DEL succeeds once, then fails because the key is gone.
    assert(storage.del("name"));
    assert(!storage.exists("name"));
    assert(!storage.get("name").has_value());
    assert(!storage.del("name"));

    std::cout << "All KeyValueStore tests passed\n";

    const std::string snapshot_file = "test_snapshot.txt";

    // Create and save the original store.
    KeyValueStore original;
    original.set("name", "Alice");
    original.set("session", "abc", std::chrono::seconds{60});
    original.save_to_file(snapshot_file);

    // Load the data into a new, empty store.
    KeyValueStore restored;
    restored.load_from_file(snapshot_file);

    // Verify the permanent entry.
    const auto name = restored.get("name");

    if (!name.has_value() || name.value() != "Alice")
    {
        std::cerr << "FAILED: permanent entry was not restored\n";
        std::filesystem::remove(snapshot_file);
        return 1;
    }

    // Verify the expiring entry.
    const auto session = restored.get("session");

    if (!session.has_value() || session.value() != "abc")
    {
        std::cerr << "FAILED: expiring entry was not restored\n";
        std::filesystem::remove(snapshot_file);
        return 1;
    }

    // Remove the test artifact.
    std::filesystem::remove(snapshot_file);

    return 0;

    // Checking that expired key is not restored 
    const std::string expired_snapshot_file =
        "expired_test_snapshot.txt";

    
        std::ofstream snapshot(expired_snapshot_file);

        if (!snapshot)
        {
            std::cerr << "FAILED: could not create expired snapshot\n";
            return 1;
        }

        snapshot << "MINI_REDIS_V1\n";
        snapshot << "\"expired\" \"gone\" 1\n";
    

    KeyValueStore expired_store;
    expired_store.load_from_file(expired_snapshot_file);

    if (expired_store.get("expired").has_value())
    {
        std::cerr << "FAILED: expired entry was restored\n";
        std::filesystem::remove(expired_snapshot_file);
        return 1;
    }

    std::filesystem::remove(expired_snapshot_file);

        // Name of the temporary snapshot file used by this test.
    const std::string corrupted_snapshot_file =
        "corrupted_test_snapshot.txt";

    {
        // Open a temporary file for writing.
        std::ofstream snapshot(corrupted_snapshot_file);

        // Stop the test if the file could not be created.
        if (!snapshot)
        {
            std::cerr << "FAILED: could not create corrupted snapshot\n";
            return 1;
        }

        // Write an invalid header intentionally.
        // load_from_file() expects "MINI_REDIS_V1".
        snapshot << "INVALID_HEADER\n";

        // This entry is valid, but the invalid header should cause
        // the loader to reject the entire snapshot before reading it.
        snapshot << "\"name\" \"Alice\" -1\n";
    }
    // The output stream is destroyed here, which closes and flushes the file.


    // Records whether load_from_file() detected the invalid snapshot.
    bool corruption_detected = false;

    try
    {
        // Create an empty store and attempt to load the corrupted file.
        KeyValueStore corrupted_store;
        corrupted_store.load_from_file(corrupted_snapshot_file);
    }
    catch (const std::runtime_error&)
    {
        // The expected exception means the corrupted header was detected.
        corruption_detected = true;
    }

    // Remove the temporary file regardless of whether loading threw.
    std::filesystem::remove(corrupted_snapshot_file);

    // If no exception occurred, the loader incorrectly accepted the file.
    if (!corruption_detected)
    {
        std::cerr << "FAILED: corrupted snapshot was accepted\n";
        return 1;
    }

        // Temporary file containing a malformed snapshot entry.
    const std::string malformed_snapshot_file =
        "malformed_test_snapshot.txt";

    {
        // Create the temporary snapshot.
        std::ofstream snapshot(malformed_snapshot_file);

        if (!snapshot)
        {
            std::cerr << "FAILED: could not create malformed snapshot\n";
            return 1;
        }

        // The header is valid, so the loader will continue to the entries.
        snapshot << "MINI_REDIS_V1\n";

        // This entry is invalid because its expiration timestamp is missing.
        // A valid line requires: key, value, and expiration timestamp.
        snapshot << "\"name\" \"Alice\"\n";
    }
    // The file is flushed and closed here.

    bool malformed_entry_detected = false;

    try
    {
        // The loader should throw std::runtime_error while parsing line 2.
        KeyValueStore malformed_store;
        malformed_store.load_from_file(malformed_snapshot_file);
    }
    catch (const std::runtime_error&)
    {
        // Throwing is the expected behavior.
        malformed_entry_detected = true;
    }

    // Clean up the temporary file.
    std::filesystem::remove(malformed_snapshot_file);

    // Fail the test if the malformed entry was silently accepted.
    if (!malformed_entry_detected)
    {
        std::cerr << "FAILED: malformed snapshot entry was accepted\n";
        return 1;
    }

}


