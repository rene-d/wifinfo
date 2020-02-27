// module téléinformation client
// rene-d 2020

#include "mock.h"

#include "config.cpp"

TEST(config, sizes)
{
    EXPECT_EQ(sizeof(EmoncmsConfig), 128);
    EXPECT_EQ(sizeof(JeedomConfig), 256);
    EXPECT_EQ(sizeof(HttpreqConfig), 256);
    EXPECT_EQ(sizeof(Config), 1024);
}

TEST(config, get_json)
{
    String r;

    config_reset();

    config_get_json(r);

    auto j1 = json::parse(r.s);

    EXPECT_EQ(j1["ssid"], config.ssid);
    EXPECT_EQ(j1["host"], config.host);
    EXPECT_EQ(j1["httpreq_port"], config.httpreq.port);
}

TEST(config, show)
{
    SerialClass::buffer.clear();
    config_show();

    EXPECT_NE(SerialClass::buffer.length(), 0u);
}

TEST(config, setup)
{
    config_setup();
    config.httpreq.port = 1515;
    config_save();
    config_read();
    EXPECT_EQ(config.httpreq.port, 1515);
}

TEST(config, form)
{
    ESP8266WebServer server;

    ESP8266WebServer::arg_called = 0;
    ESP8266WebServer::hasArg_called = 0;

    config_handle_form(server);

    EXPECT_NE(ESP8266WebServer::arg_called, 0);
    EXPECT_NE(ESP8266WebServer::hasArg_called, 0);
    EXPECT_EQ(ESP8266WebServer::send_code, 200);
}
