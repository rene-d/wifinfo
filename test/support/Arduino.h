#pragma once

#include <string>
#include <iostream>

static inline unsigned long millis() { return 1000; }
static inline unsigned long micros() { return 1000000; }
static inline void delay(unsigned) {}

static inline void pinMode(uint8_t pin, uint8_t mode) {}
static inline void digitalWrite(uint8_t pin, uint8_t val) {}
static inline int digitalRead(uint8_t pin) { return 0; }

#define LED_BUILTIN 2
#define LOW 0
#define HIGH 1
#define OUTPUT 0x01

#define F(x) x
#define FPSTR(x) x
#define PSTR(x) x
#define PROGMEM
#define sprintf_P sprintf
#define strcpy_P strcpy

struct __FlashStringHelper
{
    char *ptr;
};

class String
{
public:
    std::string s;

public:
    String() = default;

    String(int n)
    {
        s = std::to_string(n);
    }
    String(const char *o)
    {
        s.assign(o);
    }
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
};

inline String operator+(const String &o, const char *s)
{
    String r = o;
    r += s;
    return r;
}

inline std::ostream &operator<<(std::ostream &o, const String &s)
{
    o.write(s.data(), s.length());
    return o;
}

class SerialClass
{
public:
    void println() {}
    void print(const char *) {}
    void println(const char *) {}
    void print(const String &) {}
    void println(const String &) {}
    void println(uint32_t) {}
    void print(int) {}
    void printf(const char *, ...) {}
    void printf_P(const char *, ...) {}
    void flush() {}
};

extern SerialClass Serial;
