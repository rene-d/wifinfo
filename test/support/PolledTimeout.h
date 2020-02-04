#pragma once

namespace esp8266
{
namespace polledTimeout
{

class periodicMs
{
public:
    void trigger() { periodicMs_reset = true; }
    bool periodicMs_reset = false;

public:
    static const int neverExpires = 0;

    periodicMs(int)
    {
    }

    operator bool()
    {
        bool v = periodicMs_reset;
        periodicMs_reset = false;
        return v;
    }

    void resetToNeverExpires()
    {
    }

    void reset(int)
    {
    }
};

} // namespace polledTimeout

} // namespace esp8266
