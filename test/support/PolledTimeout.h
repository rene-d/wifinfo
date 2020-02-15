// module téléinformation client
// rene-d 2020

#pragma once

namespace esp8266
{
namespace polledTimeout
{

class periodicMs
{
public:
    void trigger() { periodicMs_activate = true; }
    bool periodicMs_activate = false;

public:
    static const int neverExpires = 0;

    periodicMs(int)
    {
    }

    operator bool()
    {
        bool v = periodicMs_activate;
        periodicMs_activate = false;
        return v;
    }

    void resetToNeverExpires()
    {
        periodicMs_activate = false;
    }

    void reset(int)
    {
    }
};

} // namespace polledTimeout

} // namespace esp8266
