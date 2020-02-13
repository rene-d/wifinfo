#include "mock_time.h"
#include <string.h>

static const char A_SMALL_STEP_FOR_MAN[] = "1969-07-21T02:56:00Z";

static const time_t MOCK_SECOND = 1951835;
static const int MOCK_YEAR = 215500;

int mock_gettimeofday(struct timeval *tv, void * /*tzv*/)
{
    tv->tv_sec = MOCK_SECOND;
    tv->tv_usec = 0;
    return 0;
}

struct tm *mock_localtime(const time_t *t)
{
    static struct tm tm;
    if (*t == MOCK_SECOND)
    {
        tm.tm_year = MOCK_YEAR;
        return &tm;
    }
    else
    {
        return nullptr;
    }
}

size_t mock_strftime(char *dest, size_t sz, const char *__restrict, const struct tm *tm)
{
    if (tm != nullptr && tm->tm_year == MOCK_YEAR)
    {
        strncpy(dest, A_SMALL_STEP_FOR_MAN, sz);
    }
    else
    {
        strncpy(dest, "", sz);
    }
    return 0; /* inutilisé */
}

const char *mock_time_marker()
{
    return A_SMALL_STEP_FOR_MAN;
}

time_t mock_time_timestamp()
{
    return MOCK_SECOND;
}
