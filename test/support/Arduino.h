// module téléinformation client
// rene-d 2020

#pragma once

#include <string>
#include <iostream>
#include <cstring>

extern int pinMode_called;
extern int digitalRead_called;
extern int digitalWrite_called;

static inline unsigned long millis() { return 1000; }
static inline unsigned long micros() { return 1000000; }
static inline void delay(unsigned) {}

static inline void pinMode(uint8_t pin, uint8_t mode) { ++pinMode_called; }
static inline void digitalWrite(uint8_t pin, uint8_t val) { ++digitalWrite_called; }
static inline int digitalRead(uint8_t pin)
{
    ++digitalRead_called;
    return 0;
}

#define LED_BUILTIN 2
#define LOW 0
#define HIGH 1
#define OUTPUT 0x01

#define DEC 10
#define HEX 16
#define OCT 8
#define BIN 2

typedef const char *PGM_P;
#define F(x) x
#define FPSTR(x) x
#define PSTR(x) x
#define PROGMEM
#define sprintf_P sprintf
#define strcpy_P strcpy
#define strcasecmp_P strcasecmp
#define snprintf_P snprintf

class Printable;

struct __FlashStringHelper
{
    char *ptr;
};

class String
{
public:
    std::string s;

public:
    String(const char *o = nullptr)
    {
        if (o != nullptr)
            s.assign(o);
    }
    String(const String &o) : s(o) {}
    explicit String(int n) : s(std::to_string(n)) {}
    explicit String(long n) : s(std::to_string(n)) {}
    explicit String(unsigned long n) : s(std::to_string(n)) {}
    explicit String(uint16_t n) : s(std::to_string(n)) {}
    explicit String(uint32_t n) : s(std::to_string(n)) {}
    explicit String(float, unsigned char decimalPlaces = 2);
    explicit String(double, unsigned char decimalPlaces = 2);
    explicit String(char c) { s.assign(1, c); }

    const char *c_str() const
    {
        return s.c_str();
    }

    void reserve(size_t n)
    {
        s.reserve(n);
    }

    String &operator+=(const char *o)
    {
        s.append(o);
        return *this;
    }

    String &operator+=(const String &o)
    {
        s.append(o.s);
        return *this;
    }

    String &operator+=(int n)
    {
        s.append(std::to_string(n));
        return *this;
    }

    String &operator+=(unsigned long n)
    {
        s.append(std::to_string(n));
        return *this;
    }

    String &operator+=(long n)
    {
        s.append(std::to_string(n));
        return *this;
    }

    String &operator+=(unsigned n)
    {
        s.append(std::to_string(n));
        return *this;
    }

    String &operator+=(uint16_t n)
    {
        s.append(std::to_string(n));
        return *this;
    }

    String &operator+=(char c)
    {
        s.append(1, c);
        return *this;
    }

    unsigned char concat(const __FlashStringHelper *o)
    {
        s.append(o->ptr);
        return 1;
    }

    unsigned char concat(const char *o)
    {
        s.append(o);
        return 1;
    }

    unsigned char concat(const String &o)
    {
        s.append(o.s);
        return 1;
    }

    unsigned char concat(unsigned o)
    {
        s.append(std::to_string(o));
        return 1;
    }

    void clear()
    {
        s.clear();
    }

    operator std::string() const
    {
        return s;
    }

    bool operator==(const String &o) const
    {
        return o.s == s;
    }

    bool operator==(const char *o) const
    {
        return s.compare(o) == 0;
    }

    bool operator!=(const char *o) const
    {
        return s.compare(o) != 0;
    }

    int toInt() const
    {
        return std::stoi(s);
    }

    const char *data() const
    {
        return s.data();
    }

    size_t length() const
    {
        return s.length();
    }

    char operator[](size_t p) const
    {
        return s.at(p);
    }

    char &operator[](size_t p)
    {
        return s.at(p);
    }

    void resize(size_t p)
    {
        s.resize(p);
    }

    void remove(size_t p, size_t count)
    {
        s.erase(p, p + count);
    }

    String &operator=(const char *o)
    {
        s.assign(o);
        return *this;
    }
    String &operator=(int n)
    {
        *this = String(n);
        return *this;
    }
};

inline String operator+(const String &l, const String &r)
{
    String s = l;
    s += r;
    return s;
}

inline String operator+(const String &o, const char *s)
{
    String r = o;
    r += s;
    return r;
}

inline String operator+(const char *l, const String &r)
{
    return String(l) + r;
}

inline std::ostream &operator<<(std::ostream &o, const String &s)
{
    o.write(s.data(), s.length());
    return o;
}

class Print
{
protected:
    virtual size_t write(uint8_t c) = 0;
};

class SerialClass
{
public:
    static String buffer;
    static int flush_called;

public:
    void println() { buffer += "\n"; }
    void print(const char *s) { buffer += s; }
    void println(const char *s) { buffer += String(s) + "\n"; }
    void print(const String &s) { buffer += s; }
    void println(const String &s) { buffer += s + "\n"; }
    void print(int n) { buffer += String(n); }
    void print(uint16_t n) { buffer += String(n); }
    void print(uint32_t n) { buffer += String(n); }
    void println(int n) { buffer += String(n) + "\n"; }
    void println(uint16_t n) { buffer += String(n) + "\n"; }
    void println(uint32_t n) { buffer += String(n) + "\n"; }
    void print(char c) { buffer += String(c); }
    void println(char c) { buffer += String(c) + "\n"; }
    size_t printf(const char *, ...);
    size_t printf_P(const char *, ...);
    void flush() { ++flush_called; }
};

extern SerialClass Serial;
