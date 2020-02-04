#include "sys.h"
#include "config.h"
#include "debug.h"
#include <ESP8266WiFi.h>
#include <sys/time.h>

extern "C" int clock_gettime(clockid_t unused, struct timespec *tp);

static int nb_reconnect = 0;
#define WIFINFO_VERSION "develop"

/* ======================================================================
Function: formatSize
Purpose : format a asize to human readable format
Input   : size
Output  : formated string
Comments: -
====================================================================== */
String formatSize(size_t bytes)
{
    if (bytes < 1024)
    {
        return String(bytes) + F(" byte");
    }
    else if (bytes < (1024 * 1024))
    {
        return String(bytes / 1024.0) + F(" kB");
    }
    else if (bytes < (1024 * 1024 * 1024))
    {
        return String(bytes / 1024.0 / 1024.0) + F(" MB");
    }
    else
    {
        return String(bytes / 1024.0 / 1024.0 / 1024.0) + F(" GB");
    }
}

String sys_uptime()
{
    struct timespec tp;
    clock_gettime(0, &tp);

    char buff[64];
    int sec = tp.tv_sec;
    int min = sec / 60;
    int hr = min / 60;
    long day = hr / 24;

    sprintf_P(buff, PSTR("%ld days %02d h %02d m %02d sec"), day, hr % 24, min % 60, sec % 60);
    return buff;
}

/* ======================================================================
Function: sys_get_info_json
Purpose : Return JSON string containing system data
Input   : Response String
Output  : -
Comments: -
====================================================================== */
void sys_get_info_json(String &response)
{
    response.reserve(512);

    char buffer[32];
    int32_t adc = (1000 * analogRead(A0) / 1024);

    // Json start
    response += F("[");

    response += "{\"na\":\"Uptime\",\"va\":\"";
    response += sys_uptime();
    response += "\"},";

    if (WiFi.status() == WL_CONNECTED)
    {
        response += "{\"na\":\"Wifi RSSI\",\"va\":\"";
        response += WiFi.RSSI();
        response += " dB\"},";
        response += "{\"na\":\"Wifi network\",\"va\":\"";
        response += config.ssid;
        response += "\"},";
        uint8_t mac[] = {0, 0, 0, 0, 0, 0};
        uint8_t *macread = WiFi.macAddress(mac);
        char macaddress[20];
        sprintf_P(macaddress, PSTR("%02x:%02x:%02x:%02x:%02x:%02x"), macread[0], macread[1], macread[2], macread[3], macread[4], macread[5]);
        response += "{\"na\":\"Adresse MAC station\",\"va\":\"";
        response += macaddress;
        response += "\"},";
    }
    response += "{\"na\":\"Nb reconnexions Wi-Fi\",\"va\":\"";
    response += nb_reconnect;
    response += "\"},";

    /*
    response += "{\"na\":\"Altérations Data détectées\",\"va\":\"";
    response += nb_reinit;
    response += "\"},";
    */

    response += "{\"na\":\"WifInfo Version\",\"va\":\"" WIFINFO_VERSION "\"},";

    response += "{\"na\":\"Compilé le\",\"va\":\"" __DATE__ " " __TIME__ "\"},";

    /*
    response += "{\"na\":\"Options de compilation\",\"va\":\"";
    response += optval;
    response += "\"},";
    */

    response += "{\"na\":\"SDK Version\",\"va\":\"";
    response += system_get_sdk_version();
    response += "\"},";

    response += "{\"na\":\"Chip ID\",\"va\":\"";
    sprintf_P(buffer, "0x%0X", system_get_chip_id());
    response += buffer;
    response += "\"},";

    response += "{\"na\":\"Boot Version\",\"va\":\"";
    sprintf_P(buffer, "0x%0X", system_get_boot_version());
    response += buffer;
    response += "\"},";

    response += "{\"na\":\"Flash Real Size\",\"va\":\"";
    response += formatSize(ESP.getFlashChipRealSize());
    response += "\"},";

    response += "{\"na\":\"Firmware Size\",\"va\":\"";
    response += formatSize(ESP.getSketchSize());
    response += "\"},";

    response += "{\"na\":\"Free Size\",\"va\":\"";
    response += formatSize(ESP.getFreeSketchSpace());
    response += "\"},";

    response += "{\"na\":\"Analog\",\"va\":\"";
    adc = ((1000 * analogRead(A0)) / 1024);
    sprintf_P(buffer, PSTR("%d mV"), adc);
    response += buffer;
    response += "\"},";

    FSInfo info;
    SPIFFS.info(info);

    response += "{\"na\":\"SPIFFS Total\",\"va\":\"";
    response += formatSize(info.totalBytes);
    response += "\"},";

    response += "{\"na\":\"SPIFFS Used\",\"va\":\"";
    response += formatSize(info.usedBytes);
    response += "\"},";

    response += "{\"na\":\"SPIFFS Occupation\",\"va\":\"";
    sprintf_P(buffer, "%d%%", 100 * info.usedBytes / info.totalBytes);
    response += buffer;
    response += "\"},";

    // Free mem should be last one
    response += "{\"na\":\"Free RAM\",\"va\":\"";
    response += formatSize(system_get_free_heap_size());
    response += "\"}"; // Last don't have comma at end

    // JSON end
    response += F("]");
}

// Purpose : scan Wifi Access Point and return JSON code
void sys_wifi_scan_json(String &response)
{
    response.clear();

    int8_t n = WiFi.scanNetworks();

    // JSON start
    response += F("[");

    for (uint8_t i = 0; i < n; ++i)
    {
        if (i != 0)
            response += F(",");

        response += F("{\"ssid\":\"");
        response += WiFi.SSID(i);

        response += F("\",\"rssi\":");
        response += WiFi.RSSI(i);

        response += F(",\"bssi\":\"");
        response += WiFi.BSSIDstr(i);

        response += F("\",\"channel\":");
        response += WiFi.channel(i);

        response += F(",\"encryptionType\":");
        response += WiFi.encryptionType(i);

        response += F("}");
    }

    // JSON end
    response += FPSTR("]");
}

/* ======================================================================
Function: sys_wifi_connect
Purpose : Handle Wifi connection / reconnection and OTA updates
Input   : setup true if we're called 1st Time from setup
Output  : state of the wifi status
Comments: -
====================================================================== */
int sys_wifi_connect(bool setup)
{
    int ret = WiFi.status();
    char toprint[20];
    IPAddress ad;

    if (setup)
    {
        //#ifdef DEBUG
        DebuglnF("========== WiFi diags start");
        WiFi.printDiag(Serial);
        DebuglnF("========== WiFi diags end");
        Serial.flush();
        //#endif

        // no correct SSID
        if (!*config.ssid)
        {
            DebugF("no Wifi SSID in config, trying to get SDK ones...");

            // Let's see of SDK one is okay
            if (WiFi.SSID() == "")
            {
                DebuglnF("Not found may be blank chip!");
            }
            else
            {
                *config.psk = '\0';

                // Copy SDK SSID
                strcpy(config.ssid, WiFi.SSID().c_str());

                // Copy SDK password if any
                if (WiFi.psk() != "")
                    strcpy(config.psk, WiFi.psk().c_str());

                DebuglnF("found one!");

                // save back new config
                config_save();
            }
        }

        // correct SSID
        if (*config.ssid)
        {
            uint8_t timeout;

            DebugF("Connecting to: ");
            Debugln(config.ssid);
            Serial.flush();

            // Do wa have a PSK ?
            if (*config.psk)
            {
                // protected network
                Debug(F(" with key '"));
                Debug(config.psk);
                Debug(F("'..."));
                Serial.flush();
                WiFi.begin(config.ssid, config.psk);
            }
            else
            {
                // Open network
                Debug(F("unsecure AP"));
                Serial.flush();
                WiFi.begin(config.ssid);
            }

            timeout = 50; // 50 * 200 ms = 5 sec time out
            // 200 ms loop
            while (((ret = WiFi.status()) != WL_CONNECTED) && timeout)
            {
                // Orange LED
                led_rgb_on(COLOR_ORANGE);
                delay(50);
                led_rgb_off();
                delay(150);
                --timeout;
            }
        }

        // connected ? disable AP, client mode only
        if (ret == WL_CONNECTED)
        {
            nb_reconnect++; // increase reconnections count
            DebuglnF("connected!");
            WiFi.mode(WIFI_STA);
            ad = WiFi.localIP();
            sprintf(toprint, "%d.%d.%d.%d", ad[0], ad[1], ad[2], ad[3]);
            DebugF("IP address   : ");
            Debugln(toprint);
            DebugF("MAC address  : ");
            Debugln(WiFi.macAddress());

            // not connected ? start AP
        }
        else
        {
            char ap_ssid[32];
            DebuglnF("Error!");
            Serial.flush();

            // STA+AP Mode without connected to STA, autoconnect will search
            // other frequencies while trying to connect, this is causing issue
            // to AP mode, so disconnect will avoid this

            // Disable auto retry search channel
            WiFi.disconnect();

            // SSID = hostname
            strcpy(ap_ssid, config.host);
            DebugF("Switching to AP ");
            Debugln(ap_ssid);
            Serial.flush();

            // protected network
            if (*config.ap_psk)
            {
                DebugF(" with key '");
                Debug(config.ap_psk);
                DebuglnF("'");
                WiFi.softAP(ap_ssid, config.ap_psk);
                // Open network
            }
            else
            {
                DebuglnF(" with no password");
                WiFi.softAP(ap_ssid);
            }
            WiFi.mode(WIFI_AP_STA);

            DebugF("IP address   : ");
            Debugln(WiFi.softAPIP());
            DebugF("MAC address  : ");
            Debugln(WiFi.softAPmacAddress());
        }
        // Version 1.0.7 : Use auto reconnect Wifi
        WiFi.setAutoConnect(true);
        WiFi.setAutoReconnect(true);
        DebuglnF("auto-reconnect armed !");

        // // Set OTA parameters
        // ArduinoOTA.setPort(config.ota_port);
        // ArduinoOTA.setHostname(config.host);
        // ArduinoOTA.setPassword(config.ota_auth);
        // ArduinoOTA.begin();
        // // just in case your sketch sucks, keep update OTA Available
        // // Trust me, when coding and testing it happens, this could save
        // // the need to connect FTDI to reflash
        // // Usefull just after 1st connexion when called from setup() before
        // // launching potentially buggy main()
        // for (uint8_t i = 0; i <= 10; i++)
        // {
        //     LedRGBON(COLOR_MAGENTA);
        //     delay(100);
        //     LedRGBOFF();
        //     delay(200);
        //     ArduinoOTA.handle();
        // }

    } // if setup

    return WiFi.status();
}

/* ======================================================================
Function: handleFactoryReset
Purpose : reset the module to factory settingd
Input   : -
Output  : -
Comments: -
====================================================================== */
void sys_handle_factory_reset(ESP8266WebServer &server)
{
    // Just to debug where we are
    Debugln(F("Serving /factory_reset page..."));
    config_reset();
    ESP.eraseConfig();
    server.send(200, "text/plain", FPSTR("Reset"));
    Debugln(F("Ok!"));
    delay(1000);
    ESP.restart();
    while (true)
        delay(1);
}

/* ======================================================================
Function: handleReset
Purpose : reset the module
Input   : -
Output  : -
Comments: -
====================================================================== */
void sys_handle_reset(ESP8266WebServer &server)
{
    // Just to debug where we are
    Debugln(F("Serving /reset page..."));
    server.send(200, "text/plain", FPSTR("Restart"));
    Debugln(F("Ok!"));
    delay(1000);
    ESP.restart();
    while (true)
        delay(1);
}
