// module téléinformation client
// rene-d 2020

#pragma once

#include "WiFiClient.h"

class ESP8266WebServer
{
public:
    static int send_called;
    static int send_code;
    static int hasArg_called;
    static int arg_called;

public:
    WiFiClient client()
    {
        return WiFiClient();
    }

    void send(int code, const String &, const String &)
    {
        ++send_called;
        send_code = code;
    }

    void on(const char *, ...)
    {
    }

    void on(const String &, ...)
    {
    }

    virtual String arg(const String &) const
    {
        ++arg_called;
        return "1";
    }
    virtual String arg(int) const
    {
        ++arg_called;
        return "1";
    }
    virtual bool hasArg(const String &) const
    {
        ++hasArg_called;
        return true;
    }

    virtual int args() const
    {
        return 1;
    }

    String argName(int i) const
    {
        return "argName";
    }
};
