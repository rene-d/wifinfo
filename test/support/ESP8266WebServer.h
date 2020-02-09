#pragma once

#include "WiFiClient.h"

class ESP8266WebServer
{
public:
    WiFiClient client()
    {
        return WiFiClient();
    }

    void send(int, const String &, const String &)
    {
    }

    void on(const char *, ...)
    {
    }

    void on(const String &, ...)
    {
    }

    String arg(const String &) const
    {
        return "arg";
    }

    bool hasArg(const String &) const
    {
        return true;
    }
};
