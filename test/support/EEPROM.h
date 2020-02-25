// module téléinformation client
// rene-d 2020

#pragma once

#include <vector>
#include <inttypes.h>

class EEPROMClass
{
    std::vector<uint8_t> eeprom;

public:
    EEPROMClass()
    {
        begin(1024);
    }

    void begin(uint32_t size)
    {
        eeprom.resize(size);
    }

    uint8_t read(uint32_t addr)
    {
        if ((size_t)addr < eeprom.size())
        {
            return eeprom[addr];
        }
        return 0;
    }

    void write(uint32_t addr, uint8_t byte)
    {
        if ((size_t)addr < eeprom.size())
        {
            eeprom[addr] = byte;
        }
    }

    void commit() {}
};

extern EEPROMClass EEPROM;
