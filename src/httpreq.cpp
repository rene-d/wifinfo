// **********************************************************************************
// ESP8266 Teleinfo WEB Client, web client functions
// **********************************************************************************
// Creative Commons Attrib Share-Alike License
// You are free to use/extend this library but please abide with the CC-BY-SA license:
// Attribution-NonCommercial-ShareAlike 4.0 International License
// http://creativecommons.org/licenses/by-nc-sa/4.0/
//
// For any explanation about teleinfo ou use, see my blog
// http://hallard.me/category/tinfo
//
// This program works with the Wifinfo board
// see schematic here https://github.com/hallard/teleinfo/tree/master/Wifinfo
//
// Written by Charles-Henri Hallard (http://hallard.me)
//
// History : V1.00 2015-12-04 - First release
//
// All text above must be included in any redistribution.
//
// **********************************************************************************

#include "httpreq.h"
#include <ESP8266HTTPClient.h>

 void http_request(const char *host, uint16_t port, const String &url)
{
    WiFiClient client;
    HTTPClient http;

    unsigned long start = millis();

    http.begin(client, host, port, url);

    Serial.printf("http%s://%s:%d%s => ", port == 443 ? "s" : "", host, port, url.c_str());

    // start connection and send HTTP header
    int http_code = http.GET();

    Serial.printf("%d in %lu ms\n", http_code, millis() - start);
}
