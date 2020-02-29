// module téléinformation client
// rene-d 2020

#include "features.h"

#ifdef ENABLE_CPULOAD

#include "cpuload.h"
#include <PolledTimeout.h>
#include <Arduino.h>

#define MAX_COUNTER_80MHZ 85000 // observed value at 80MHz without any processing

static uint32_t perf_counter = 0;
static esp8266::polledTimeout::periodicMs timer_perf(1000);
static uint32_t perfs[10];
static int perf_index = 0;

void cpuload_loop()
{
    ++perf_counter;

    if (timer_perf)
    {
        perfs[perf_index] = perf_counter;
        perf_index = (perf_index + 1) % 10;

        perf_counter = 0;

#ifdef ENABLE_DEBUG
        if (perf_index == 0)
        {
            cpuload_print(Serial);
        }
#endif
    }
}

int cpuload_cpu()
{
    uint32_t sum = 0;
    for (int i = 0; i < 10; ++i)
    {
        sum += perfs[i];
    }
    sum /= 10;
    return (100 * (MAX_COUNTER_80MHZ - sum)) / MAX_COUNTER_80MHZ;
}

void cpuload_print(Print &prt)
{
    uint32_t sum = 0;
    for (int i = 0; i < 10; ++i)
    {
        sum += perfs[i];
    }
    sum /= 10;

    prt.printf("PERF: millis=%10lu counter=%6u/sec load=%d %%\n",
               millis(), sum,
               (100 * (MAX_COUNTER_80MHZ - sum)) / MAX_COUNTER_80MHZ);
}

#endif
