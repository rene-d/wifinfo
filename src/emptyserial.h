// module téléinformation client
// rene-d 2020

#pragma once

#include <Arduino.h>

struct EmptySerialClass
{
    void printf(const char *format, ...) {}
    void printf_P(PGM_P format, ...) {}
    void print(const __FlashStringHelper *) {}
    void print(const String &) {}
    void print(const char[]) {}
    void print(char) {}
    void print(unsigned char, int = DEC) {}
    void print(int, int = DEC) {}
    void print(unsigned int, int = DEC) {}
    void print(long, int = DEC) {}
    void print(unsigned long, int = DEC) {}
    void print(double, int = 2) {}
    void print(const Printable &) {}

    void println(const __FlashStringHelper *) {}
    void println(const String &s) {}
    void println(const char[]) {}
    void println(char) {}
    void println(unsigned char, int = DEC) {}
    void println(int, int = DEC) {}
    void println(unsigned int, int = DEC) {}
    void println(long, int = DEC) {}
    void println(unsigned long, int = DEC) {}
    void println(double, int = 2) {}
    void println(const Printable &) {}
    void println(void) {}

    void flush() {}
};

#ifndef DEBUG
extern EmptySerialClass EmptySerial;
#define Serial EmptySerial
#endif
