#pragma once

#include <Arduino.h>

void fs_setup();
void fs_get_spiffs_json(String &response);
void fs_ls();
