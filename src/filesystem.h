// module téléinformation client
// rene-d 2020

#pragma once

#include <Arduino.h>

void fs_setup();
void fs_get_spiffs_json(String &response);
void fs_ls();
