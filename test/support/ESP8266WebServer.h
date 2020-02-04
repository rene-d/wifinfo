#pragma once

#include "WiFiClient.h"

class ESP8266WebServer
{
public:
    WiFiClient client()
    {
        return WiFiClient();
    }

    void send(int, const char *, const char *)
    {
    }

    void on(const char *, ...)
    {
    }

    void on(const String &, ...)
    {
    }
};
