#pragma once

class ESPClass
{
public:
    uint32_t getChipId() const
    {
        return 0x123ABC;
    }

    uint32_t getSketchSize() const
    {
        return 314159;
    }

    uint32_t getFreeSketchSpace() const
    {
        return 2718281;
    }

    uint32_t getFlashChipRealSize() const
    {
        return 0x400000;
    }

    void restart()
    {
    }

    void eraseConfig()
    {
    }
};

void system_update_cpu_freq(uint8_t);

uint32_t system_get_free_heap_size();

const char *system_get_sdk_version();

uint8_t system_get_boot_version(void);

uint32_t system_get_chip_id();

extern ESPClass ESP;
