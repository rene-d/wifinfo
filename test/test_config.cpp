#include "mock.h"

#include "config.h"

TEST(config, sizes)
{
    ASSERT_EQ(sizeof(_emoncms), 128);
    ASSERT_EQ(sizeof(_jeedom), 256);
    ASSERT_EQ(sizeof(_httpRequest), 256);
    ASSERT_EQ(sizeof(_Config), 1024);
}
