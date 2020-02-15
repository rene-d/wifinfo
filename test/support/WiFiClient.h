#pragma once

#include <Arduino.h>

class IPAddress
{
public:
    String toString() const
    {
        return "1.1.1.1";
    }
};

class WiFiClient
{
public:
    void println(const char *) {}
    void println() {}
    void print(const char *) {}
    void print(const String &) {}
    void flush() {}
    void stop() {}
    bool connected()
    {
        return true;
    }
    operator bool() const
    {
        return true;
    }
    IPAddress remoteIP()
    {
        return IPAddress();
    }
    uint16_t remotePort()
    {
        return 13270;
    }
};
