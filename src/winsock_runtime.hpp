#pragma once

// Manages the lifetime of the Windows Sockets library.
class WinsockRuntime
{
public:
    WinsockRuntime();
    ~WinsockRuntime();

    // A Winsock session should have one clear owner.
    WinsockRuntime(const WinsockRuntime&) = delete;
    WinsockRuntime& operator=(const WinsockRuntime&) = delete;
    
    // Moving is also disabled to keep ownership simple and explicit.
    WinsockRuntime(WinsockRuntime&&) = delete;
    WinsockRuntime& operator=(WinsockRuntime&&) = delete;
};