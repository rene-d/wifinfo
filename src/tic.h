// module téléinformation client
// rene-d 2020

#pragma once

#include <Arduino.h>

void tic_decode(int c);
void tic_make_timers();
void tic_notifs();

const char *tic_get_value(const char *label);
void tic_get_json_array(String &html, bool restricted);
void tic_get_json_dict(String &html, bool restricted);
void tic_emoncms_data(String &url, bool restricted);

void tic_dump();
