#pragma once

#include <vector>

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
        if (addr >= eeprom.size())
            return 0;
        return eeprom[addr];
    }

    void write(uint32_t addr, uint8_t byte)
    {
        if (addr < eeprom.size())
            eeprom[addr] = byte;
    }

    void commit() {}
};

extern EEPROMClass EEPROM;
