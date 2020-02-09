// very simple JSON "flat" builder

#pragma once

#include <Arduino.h>

class JSONBuilder
{
    String &s;

public:
    JSONBuilder(String &r, size_t reserve_size = 256) : s(r)
    {
        s.reserve(reserve_size);
        s.clear();
        s.concat("{\"");
    }

    void append(const __FlashStringHelper *name, const char *value, bool last = false)
    {
        s.concat(name);
        s.concat("\":\"");
        s.concat(value);

        if (last)
            s.concat("\"}");
        else
            s.concat("\",\"");
    }

    void append(const __FlashStringHelper *name, uint32_t value, bool last = false)
    {
        s.concat(name);
        s.concat("\":");
        s.concat(value);

        if (last)
            s.concat("}");
        else
            s.concat(",\"");
    }

    void append(const char *name, const char *value, bool last = false)
    {
        s.concat(name);
        s.concat("\":\"");
        s.concat(value);

        if (last)
            s.concat("\"}");
        else
            s.concat("\",\"");
    }

    void append_without_quote(const char *name, const char *value, bool last = false)
    {
        s.concat(name);
        s.concat("\":");
        s.concat(value);

        if (last)
            s.concat("}");
        else
            s.concat(",\"");
    }

    void append(const char *name, uint32_t value, bool last = false)
    {
        s.concat(name);
        s.concat("\":");
        s.concat(value);

        if (last)
            s.concat("}");
        else
            s.concat(",\"");
    }

    void finalize()
    {
        // nota:
        //  s a toujours au moins 2 caractères:
        //  {"  ou se termine par ",

        s.remove(s.length() - 1, 1); // supprime " ou , de fin

        char &last = s[s.length() - 1];
        if (last == '{')
        {
            // dict vide
            s.concat('}');
        }
        else
        {
            // remplace la , de fin par }
            last = '}';
        }
    }
};

class JSONTableBuilder
{
    String &s;

public:
    JSONTableBuilder(String &r, size_t reserve_size = 256) : s(r)
    {
        s.reserve(reserve_size);
        s.clear();
        s.concat("[");
    }

    void append(const char *name, const char *value)
    {
        s.concat("{\"na\":\"");
        s.concat(name);
        s.concat("\",\"va\":\"");
        s.concat(value);
        s.concat("\"},");
    }

    void append(const char *name, const String &value)
    {
        append(name, value.c_str());
    }

    void append(const char *name, uint32_t value)
    {
        s.concat("{\"na\":\"");
        s.concat(name);
        s.concat("\",\"va\":");
        s.concat(value);
        s.concat("},");
    }

    void finalize()
    {
        // s a toujours au moins 1 caractère
        char &last = s[s.length() - 1];
        if (last == '[')
            s.concat(']');
        else
            last = ']';
    }
};
