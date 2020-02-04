#pragma once

#include <string>

static inline unsigned long millis() { return 1000; }
static inline void delay(unsigned) {}

#define F(x) x

class SerialClass
{
public:
    void println() {}
    void print(const char *) {}
    void println(const char *) {}
    void print(int) {}
    void printf(const char *, ...) {}
    void flush() {}
};

extern SerialClass Serial;

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
};
