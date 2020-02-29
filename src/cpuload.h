// module téléinformation client
// rene-d 2020

#pragma once

#include <Arduino.h>

void cpuload_loop();
void cpuload_print(Print &prt);
int cpuload_cpu();

class StringPrint : public Print
{
protected:
    String buffer_;

    virtual size_t write(uint8_t c) override
    {
        buffer_.concat((char)c);
        return 1;
    }

public:
    operator const String &() const
    {
        return buffer_;
    }
};
