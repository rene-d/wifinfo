#include "mock.h"

#include "config.cpp"

TEST(config, sizes)
{
    ASSERT_EQ(sizeof(_emoncms), 128);
    ASSERT_EQ(sizeof(_jeedom), 256);
    ASSERT_EQ(sizeof(_httpRequest), 256);
    ASSERT_EQ(sizeof(_Config), 1024);
}

TEST(config, get_json)
{
    String r;

    config_reset();

    config_get_json(r);

    auto j1 = json::parse(r.s);

    ASSERT_EQ(j1["ssid"], config.ssid);
    ASSERT_EQ(j1["host"], config.host);
    ASSERT_EQ(j1["httpreq_port"], config.httpReq.port);
}
