#pragma once

#include <time.h>
#include <sys/time.h>

int mock_gettimeofday(struct timeval *tv, void * /*tzv*/);
struct tm *mock_localtime(const time_t *t);
size_t mock_strftime(char *dest, size_t sz, const char *__restrict, const struct tm *tm);

#define gettimeofday mock_gettimeofday
#define localtime mock_localtime
#define strftime mock_strftime

const char *mock_time_marker();
time_t mock_time_timestamp();
