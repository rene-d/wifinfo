#pragma once

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <gtest/gtest.h>     // https://github.com/google/googletest
#include <nlohmann/json.hpp> // https://github.com/nlohmann/json
using json = nlohmann::json;

//
const char *test_time_marker();

extern const std::string trame_teleinfo;

void tinfo_init();
void tinfo_init(uint32_t papp, bool heures_creuses, uint32_t adps = 0);
