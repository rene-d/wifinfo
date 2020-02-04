#pragma once

#include <Arduino.h>

void tic_decode(int c);
void tic_make_timers();
void tic_notifs();

const char *tic_get_value(const char *label);
void tic_get_json_array(String &html);
void tic_get_json_dict(String &html);
void tic_emoncms_data(String& url);
