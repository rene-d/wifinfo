#pragma once

#include <Arduino.h>

class WiFiClient
{
public:
    void println(const char *) {}
    void println() {}
    void print(const char *) {}
    void print(const String& ) {}
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
};
