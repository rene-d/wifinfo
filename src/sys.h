#pragma once

#include <Arduino.h>
#include <ESP8266WebServer.h>

void sys_get_info_json(String &response);
void sys_handle_reset(ESP8266WebServer &server);
void sys_handle_factory_reset(ESP8266WebServer &server);
void sys_wifi_scan_json(String& response);
int sys_wifi_connect(bool setup = false);
