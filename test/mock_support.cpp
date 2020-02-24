// module téléinformation client
// rene-d 2020

#include <ESP8266HTTPClient.h>
#include <PolledTimeout.h>
#include <user_interface.h>
#include <EEPROM.h>
#include <ESP8266WiFi.h>
#include <FS.h>
#include <Arduino.h>

ESPClass ESP;
EEPROMClass EEPROM;
WiFiClass WiFi;
SerialClass Serial;
SPIFFSClass SPIFFS;

int HTTPClient::begin_called = 0;
String HTTPClient::begin_host;
uint16_t HTTPClient::begin_port = 0;
String HTTPClient::begin_url;
bool HTTPClient::begin_https = false;

void HTTPClient::begin(WiFiClient &, const char *host, uint16_t port, const String &url, bool https)
{
    begin_called++;
    begin_host = host;
    begin_port = port;
    begin_url = url;
    begin_https = https;
}

void system_update_cpu_freq(uint8_t)
{
}

uint32_t system_get_free_heap_size()
{
    return 36123u;
}

const char *system_get_sdk_version()
{
    return "esp8266-gtest";
}

uint8_t system_get_boot_version(void)
{
    return 1;
}

uint32_t system_get_chip_id()
{
    return ESP.getChipId();
}
