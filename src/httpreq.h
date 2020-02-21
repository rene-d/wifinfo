// module téléinformation client
// rene-d 2020

#pragma once

#include <Arduino.h>
#include <inttypes.h>

void http_request(const char *host, uint16_t port, const String &url, const char *data = nullptr);
