#pragma once

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <gtest/gtest.h>
#include <nlohmann/json.hpp> // https://github.com/nlohmann/json
using json = nlohmann::json;

///////////

//
const char *test_time_marker();

extern const std::string trame_teleinfo;

void tinfo_init();
void tinfo_init(uint32_t papp, bool heures_creuses);
