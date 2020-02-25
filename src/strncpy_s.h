#pragma once

static inline void strncpy_s(char *dest, const String &source, size_t size_minus_one)
{
    strncpy(dest, source.c_str(), size_minus_one);
    dest[size_minus_one] = '\0';
}
