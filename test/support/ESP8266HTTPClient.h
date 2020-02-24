// module téléinformation client
// rene-d 2020

#pragma once

#include <Arduino.h>
#include "WiFiClient.h"

class HTTPClient
{
public:
    static int begin_called;
    static String begin_host;
    static uint16_t begin_port;
    static String begin_url;
    static bool begin_https;
    static int GET_called;
    static int POST_called;
    static int addHeader_called;
    static String POST_data;

public:
    void begin(WiFiClient &, const char *host, uint16_t port, const String &url, bool https = false)
    {
        ++begin_called;
        begin_host = host;
        begin_port = port;
        begin_url = url;
        begin_https = https;
    }

    int GET()
    {
        ++GET_called;
        return 0;
    }

    int POST(const String &data)
    {
        ++POST_called;
        POST_data = data;
        return 0;
    }

    void addHeader(const char *, const char *)
    {
        ++addHeader_called;
    }
};
