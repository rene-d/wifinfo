#include <ESP8266HTTPClient.h>
#include <PolledTimeout.h>
#include <user_interface.h>
#include <EEPROM.h>
#include <ESP8266WiFi.h>
#include <FS.h>

ESPClass ESP;
EEPROMClass EEPROM;
WiFiClass WiFi;
SerialClass Serial;
SPIFFSClass SPIFFS;

int HTTPClient::begin_called = 0;
std::string HTTPClient::begin_host;
uint16_t HTTPClient::begin_port;
std::string HTTPClient::begin_url;

void HTTPClient::begin(WiFiClient &, const char *host, uint16_t port, const String &url)
{
    begin_called++;
    begin_host = host;
    begin_port = port;
    begin_url = url;
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
