#pragma once

#include <Arduino.h>
#include <inttypes.h>

void http_request(const char *host, uint16_t port, const String &url);
