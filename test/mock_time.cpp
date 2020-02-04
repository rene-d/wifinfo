#include <string.h>
#include <time.h>
#include <sys/time.h>


static const char A_SMALL_STEP_FOR_MAN[] = "1969-07-21T02:56:00Z";

int gettimeofday(struct timeval *tv, void * /*tzv*/)
{
    tv->tv_sec = 1951835;
    tv->tv_usec = 0;
    return 0;
}

struct tm *localtime(const time_t *t)
{
    static struct tm tm;
    if (*t == 1951835)
    {
        tm.tm_year = 2155;
        return &tm;
    }
    else
    {
        return nullptr;
    }
}

size_t strftime(char *dest, size_t sz, const char *__restrict, const struct tm *tm)
{
    if (tm != nullptr && tm->tm_year == 2155)
    {
        strncpy(dest, A_SMALL_STEP_FOR_MAN, sz);
    }
    else
    {
        strncpy(dest, "", sz);
    }
    return 0; /* inutilisé */
}

const char *test_time_marker()
{
    return A_SMALL_STEP_FOR_MAN;
}
