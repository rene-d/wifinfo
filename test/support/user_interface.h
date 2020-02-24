// module téléinformation client
// rene-d 2020

#pragma once

class ESPClass
{
public:
    static int restart_called;
    static int eraseConfig_called;

public:
    uint32_t getChipId() const
    {
        return 0x12AB;
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
        ++restart_called;
    }

    void eraseConfig()
    {
        ++eraseConfig_called;
    }
};

void system_update_cpu_freq(uint8_t);

uint32_t system_get_free_heap_size();

const char *system_get_sdk_version();

uint8_t system_get_boot_version(void);

uint32_t system_get_chip_id();

extern ESPClass ESP;
