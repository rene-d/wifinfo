// module téléinformation client
// rene-d 2020

#include "mock.h"

#define ENABLE_LED
#include "sys.cpp"

TEST(sys, format)
{
    EXPECT_EQ(sys_format_size(1).s, "1 byte");
    EXPECT_EQ(sys_format_size(1536).s, "1.50 kB");
    EXPECT_EQ(sys_format_size(10485760).s, "10.00 MB");
    EXPECT_EQ(sys_format_size(3221225472u).s, "3.00 GB");
}

TEST(sys, get_json)
{
    String data;

    sys_get_info_json(data, false);

    // std::cout << data << std::endl;

    auto j1 = json::parse(data.s);

    ASSERT_TRUE(j1.is_array());
}

TEST(sys, wifi_scan)
{
    String response;

    sys_wifi_scan_json(response, false);

    // std::cout << response << std::endl;

    auto j1 = json::parse(response.s);

    ASSERT_TRUE(j1.is_array());
    ASSERT_EQ(j1.size(), 2u);

    ASSERT_EQ(j1[0].size(), 5u);
    ASSERT_EQ(j1[0]["ssid"], "superwifi");
    ASSERT_EQ(j1[0]["rssi"], -72);
    ASSERT_EQ(j1[0]["bssi"], "blah");
    ASSERT_EQ(j1[0]["channel"], 1);
    ASSERT_EQ(j1[0]["encryptionType"], 8);

    ASSERT_EQ(j1[1].size(), 5u);
    ASSERT_EQ(j1[1]["ssid"], "mauvaiswifi");
    ASSERT_EQ(j1[1]["encryptionType"], 6);
}

TEST(sys, factory_reset)
{
    ESP8266WebServer server;

    ESP8266WebServer::send_called = 0;
    ESPClass::restart_called = 0;
    ESPClass::eraseConfig_called = 0;

    sys_handle_factory_reset(server);

    ASSERT_EQ(ESP8266WebServer::send_called, 1);
    ASSERT_EQ(ESPClass::restart_called, 1);
    ASSERT_EQ(ESPClass::eraseConfig_called, 1);
}

TEST(sys, reset)
{
    ESP8266WebServer server;

    ESP8266WebServer::send_called = 0;
    ESPClass::restart_called = 0;
    ESPClass::eraseConfig_called = 0;

    sys_handle_reset(server);

    ASSERT_EQ(ESP8266WebServer::send_called, 1);
    ASSERT_EQ(ESPClass::restart_called, 1);
    ASSERT_EQ(ESPClass::eraseConfig_called, 0);
}

TEST(led, enabled)
{
    pinMode_called = 0;
    digitalRead_called = 0;
    digitalWrite_called = 0;

    led_setup();
    ASSERT_EQ(pinMode_called, 1); // uniquement un appel à pinMode
    ASSERT_EQ(digitalRead_called, 0);
    ASSERT_EQ(digitalWrite_called, 0);

    led_on();
    ASSERT_EQ(pinMode_called, 1);
    ASSERT_EQ(digitalRead_called, 0);
    ASSERT_EQ(digitalWrite_called, 1); // uniquement un appel à digitalWrite

    led_off();
    ASSERT_EQ(pinMode_called, 1);
    ASSERT_EQ(digitalRead_called, 0);
    ASSERT_EQ(digitalWrite_called, 2); // uniquement un appel à digitalWrite

    led_toggle();
    ASSERT_EQ(pinMode_called, 1);
    ASSERT_EQ(digitalRead_called, 1);  // un appel à digitalRead
    ASSERT_EQ(digitalWrite_called, 3); // et un appel à digitalWrite
}

TEST(sys, get_json_restricted)
{
    String data;

    sys_get_info_json(data, true);
    // std::cout << data << std::endl;

    EXPECT_EQ(data.s.find("Wi-Fi network"), std::string::npos);
    EXPECT_NE(data.s.find("Uptime"), std::string::npos);

    auto j1 = json::parse(data.s);

    ASSERT_TRUE(j1.is_array());
}

TEST(sys, wifi_scan_restricted)
{
    String response;

    sys_wifi_scan_json(response, true);

    EXPECT_EQ(response.s, "{}");
}
