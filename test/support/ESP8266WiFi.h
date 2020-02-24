// module téléinformation client
// rene-d 2020

#pragma once

#include <Arduino.h>

#define WL_CONNECTED 0
#define WIFI_STA 1
#define WIFI_AP_STA 2

class WiFiClass
{
    uint8_t mac[6]{0, 0, 0, 1, 2, 3};

public:
    int status()
    {
        return WL_CONNECTED;
    }

    void disconnect()
    {
    }

    int8_t scanNetworks()
    {
        return 2;
    }

    void begin(const char *ssid, const char *psk = nullptr)
    {
    }

    void softAP(const char *ssid, const char *psk = nullptr)
    {
    }

    void mode(int)
    {
    }

    void setAutoConnect(bool)
    {
    }

    void setAutoReconnect(bool)
    {
    }

    int RSSI(uint8_t n = 0) const
    {
        return n == 0 ? -72 : -99;
    }

    String SSID(uint8_t n = 0) const
    {
        return n == 0 ? "superwifi" : "mauvaiswifi";
    }

    String psk(uint8_t n = 0) const
    {
        return n == 0 ? "supermotdepasse" : "azerty";
    }

    String BSSIDstr(uint8_t n = 0) const
    {
        return n == 0 ? "blah" : "lol";
    }

    uint8_t channel(uint8_t n = 0) const
    {
        return n == 0 ? 1 : 2;
    }

    uint8_t encryptionType(uint8_t n = 0) const
    {
        return n == 0 ? 8 : 6;
    }

    String macAddress() const
    {
        return "00:11:22:33:44:55";
    }

    String softAPmacAddress() const
    {
        return "AA:BB:CC:DD:EE:FF";
    }

    uint8_t *macAddress(uint8_t *)
    {
        return mac;
    }

    String softAPIP() const
    {
        return "192.168.4.1";
    }

    String localIP() const
    {
        return "192.168.1.2";
    }

    void printDiag(SerialClass &)
    {
    }
};

extern WiFiClass WiFi;
