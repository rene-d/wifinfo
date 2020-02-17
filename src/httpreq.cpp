// module téléinformation client
// rene-d 2020

#include "httpreq.h"

#include <ESP8266HTTPClient.h>

#include "emptyserial.h"

void http_request(const char *host, uint16_t port, const String &url)
{
    WiFiClient client;
    HTTPClient http;

    unsigned long start = micros();

    http.begin(client, host, port, url);

    Serial.printf("http://%s:%d%s => ", host, port, url.c_str());

    int http_code = http.GET();

    Serial.printf("%d in %lu us\n", http_code, micros() - start);
}
