// module téléinformation client
// rene-d 2020

#pragma once

#include "WiFiClient.h"

class ESP8266WebServer
{
public:
    static int send_called;

public:
    WiFiClient client()
    {
        return WiFiClient();
    }

    void send(int, const String &, const String &)
    {
        ++send_called;
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
    String arg(int) const
    {
        return "arg";
    }
    bool hasArg(const String &) const
    {
        return true;
    }

    int args() const
    {
        return 1;
    }

    String argName(int i) const
    {
        return "argName";
    }
};
