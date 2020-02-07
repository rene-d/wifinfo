#include "sys.h"
#include "config.h"
#include "led.h"
#include <ESP8266WiFi.h>
#include <ArduinoOTA.h>
#include <ESP8266WebServer.h>
#include <sys/time.h>

extern "C" int clock_gettime(clockid_t unused, struct timespec *tp);

#ifndef WIFINFO_VERSION
#define WIFINFO_VERSION "develop"
#endif

static int nb_reconnect = 0;
static bool ota_blink = false;

// format a size to human readable format
String sys_format_size(size_t bytes)
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

// Return JSON string containing system data
void sys_get_info_json(String &response)
{
    response.reserve(512);

    char buffer[32];

    // JSON start
    response += F("[");

    response += F("{\"na\":\"Uptime\",\"va\":\"");
    response += sys_uptime();
    response += F("\"},");

    if (WiFi.status() == WL_CONNECTED)
    {
        response += "{\"na\":\"Wi-Fi RSSI\",\"va\":\"";
        response += WiFi.RSSI();
        response += " dB\"},";
        response += "{\"na\":\"Wi-Fi network\",\"va\":\"";
        response += config.ssid;
        response += F("\"},");
        uint8_t mac[] = {0, 0, 0, 0, 0, 0};
        uint8_t *macread = WiFi.macAddress(mac);
        char macaddress[20];
        sprintf_P(macaddress, PSTR("%02x:%02x:%02x:%02x:%02x:%02x"), macread[0], macread[1], macread[2], macread[3], macread[4], macread[5]);
        response += F("{\"na\":\"Adresse MAC station\",\"va\":\"");
        response += macaddress;
        response += F("\"},");
    }
    response += F("{\"na\":\"Nb reconnexions Wi-Fi\",\"va\":\"");
    response += nb_reconnect;
    response += F("\"},");

    response += F("{\"na\":\"WifInfo Version\",\"va\":\"" WIFINFO_VERSION "\"},");

    response += F("{\"na\":\"Compilé le\",\"va\":\"" __DATE__ " " __TIME__ "\"},");

    response += F("{\"na\":\"SDK Version\",\"va\":\"");
    response += system_get_sdk_version();
    response += F("\"},");

    response += F("{\"na\":\"Chip ID\",\"va\":\"");
    sprintf_P(buffer, "0x%0X", system_get_chip_id());
    response += buffer;
    response += F("\"},");

    response += F("{\"na\":\"Boot Version\",\"va\":\"");
    sprintf_P(buffer, "0x%0X", system_get_boot_version());
    response += buffer;
    response += F("\"},");

    response += F("{\"na\":\"Flash Real Size\",\"va\":\"");
    response += sys_format_size(ESP.getFlashChipRealSize());
    response += F("\"},");

    response += F("{\"na\":\"Firmware Size\",\"va\":\"");
    response += sys_format_size(ESP.getSketchSize());
    response += F("\"},");

    response += F("{\"na\":\"Free Size\",\"va\":\"");
    response += sys_format_size(ESP.getFreeSketchSpace());
    response += F("\"},");

    /*
    response += F("{\"na\":\"Analog\",\"va\":\"");
    int adc = ((1000 * analogRead(A0)) / 1024);
    sprintf_P(buffer, PSTR("%d mV"), adc);
    response += buffer;
    response += F("\"},");
    */

    FSInfo info;
    SPIFFS.info(info);

    response += F("{\"na\":\"SPIFFS Total\",\"va\":\"");
    response += sys_format_size(info.totalBytes);
    response += F("\"},");

    response += F("{\"na\":\"SPIFFS Used\",\"va\":\"");
    response += sys_format_size(info.usedBytes);
    response += F("\"},");

    response += F("{\"na\":\"SPIFFS Occupation\",\"va\":\"");
    sprintf_P(buffer, "%d%%", 100 * info.usedBytes / info.totalBytes);
    response += buffer;
    response += F("\"},");

    // Free mem should be last one
    response += F("{\"na\":\"Free RAM\",\"va\":\"");
    response += sys_format_size(system_get_free_heap_size());
    response += F("\"}"); // Last don't have comma at end

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

// Handle Wifi connection / reconnection and OTA updates
int sys_wifi_connect()
{
    int ret = WiFi.status();
    IPAddress ad;

    //#ifdef DEBUG
    Serial.println("========== WiFi diags start");
    WiFi.printDiag(Serial);
    Serial.println("========== WiFi diags end");
    Serial.flush();
    //#endif

    // no correct SSID
    if (!*config.ssid)
    {
        Serial.print(F("no Wifi SSID in config, trying to get SDK ones..."));

        // Let's see of SDK one is okay
        if (WiFi.SSID() == "")
        {
            Serial.println(F("Not found may be blank chip!"));
        }
        else
        {
            *config.psk = '\0';

            // Copy SDK SSID
            strcpy(config.ssid, WiFi.SSID().c_str());

            // Copy SDK password if any
            if (WiFi.psk() != "")
                strcpy(config.psk, WiFi.psk().c_str());

            Serial.println("found one!");

            // save back new config
            config_save();
        }
    }

    // correct SSID
    if (*config.ssid)
    {
        uint8_t timeout;

        Serial.print(F("Connecting to: "));
        Serial.println(config.ssid);
        Serial.flush();

        // Do wa have a PSK ?
        if (*config.psk)
        {
            // protected network
            Serial.print(F(" with key '"));
            Serial.print(config.psk);
            Serial.print(F("'..."));
            Serial.flush();
            WiFi.begin(config.ssid, config.psk);
        }
        else
        {
            // Open network
            Serial.print(F("unsecure AP"));
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
        Serial.println(F("connected!"));
        WiFi.mode(WIFI_STA);
        ad = WiFi.localIP();
        Serial.print(F("IP address   : "));
        Serial.printf("%d.%d.%d.%d", ad[0], ad[1], ad[2], ad[3]);
        Serial.print(F("MAC address  : "));
        Serial.println(WiFi.macAddress());

        // not connected ? start AP
    }
    else
    {
        char ap_ssid[32];
        Serial.println("Error!");
        Serial.flush();

        // STA+AP Mode without connected to STA, autoconnect will search
        // other frequencies while trying to connect, this is causing issue
        // to AP mode, so disconnect will avoid this

        // Disable auto retry search channel
        WiFi.disconnect();

        // SSID = hostname
        strcpy(ap_ssid, config.host);
        Serial.print(F("Switching to AP "));
        Serial.println(ap_ssid);
        Serial.flush();

        // protected network
        if (*config.ap_psk)
        {
            Serial.print(F(" with key '"));
            Serial.print(config.ap_psk);
            Serial.println(F("'"));
            WiFi.softAP(ap_ssid, config.ap_psk);
            // Open network
        }
        else
        {
            Serial.println(F(" with no password"));
            WiFi.softAP(ap_ssid);
        }
        WiFi.mode(WIFI_AP_STA);

        Serial.print(F("IP address   : "));
        Serial.println(WiFi.softAPIP());
        Serial.print(F("MAC address  : "));
        Serial.println(WiFi.softAPmacAddress());
    }
    // Version 1.0.7 : Use auto reconnect Wifi
    WiFi.setAutoConnect(true);
    WiFi.setAutoReconnect(true);
    Serial.println(F("auto-reconnect armed !"));

    // // Set OTA parameters
    ArduinoOTA.setPort(config.ota_port);
    ArduinoOTA.setHostname(config.host);
    ArduinoOTA.setPassword(config.ota_auth);
    ArduinoOTA.begin();

    // just in case your sketch sucks, keep update OTA Available
    // Trust me, when coding and testing it happens, this could save
    // the need to connect FTDI to reflash
    // Useful just after 1st connection when called from setup() before
    // launching potentially buggy main()
    for (uint8_t i = 0; i <= 10; i++)
    {
        led_rgb_on(COLOR_MAGENTA);
        delay(100);
        led_rgb_off();
        delay(200);
        ArduinoOTA.handle();
    }

    return WiFi.status();
}

// reset the module to factory settingd
void sys_handle_factory_reset(ESP8266WebServer &server)
{
    // Just to debug where we are
    Serial.println(F("Serving /factory_reset page..."));
    config_reset();
    ESP.eraseConfig();
    server.send(200, "text/plain", FPSTR("Reset"));
    Serial.println(F("Ok!"));
    delay(1000);
    ESP.restart();
    while (true)
        delay(1);
}

// reset the module
void sys_handle_reset(ESP8266WebServer &server)
{
    // Just to debug where we are
    Serial.println(F("Serving /reset page..."));
    server.send(200, "text/plain", FPSTR("Restart"));
    Serial.println(F("Ok!"));
    delay(1000);
    ESP.restart();
    while (true)
        delay(1);
}

void sys_ota_setup()
{
    // OTA callbacks
    ArduinoOTA.onStart([]() {
        led_rgb_on(COLOR_MAGENTA);
        Serial.println(F("Update Started"));
        ota_blink = true;
    });

    ArduinoOTA.onEnd([]() {
        led_rgb_off();
        Serial.println(F("Update finished : restarting"));
    });

    ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
        if (ota_blink)
        {
            led_rgb_on(COLOR_MAGENTA);
        }
        else
        {
            led_rgb_off();
        }
        ota_blink = !ota_blink;
        //Serial.printf("Progress: %u%%\n", (progress / (total / 100)));
    });

    ArduinoOTA.onError([](ota_error_t error) {
        led_rgb_on(COLOR_RED);
#ifdef DEBUG
        Serial.printf("Update Error[%u]: ", error);
        if (error == OTA_AUTH_ERROR)
            Serial.println("Auth Failed");
        else if (error == OTA_BEGIN_ERROR)
            Serial.println("Begin Failed");
        else if (error == OTA_CONNECT_ERROR)
            Serial.println("Connect Failed");
        else if (error == OTA_RECEIVE_ERROR)
            Serial.println("Receive Failed");
        else if (error == OTA_END_ERROR)
            Serial.println("End Failed");
#endif
        ESP.restart();
    });
}

void sys_ota_register(ESP8266WebServer &server)
{
    // handler for the /update form POST (once file upload finishes)
    server.on(
        "/update",
        HTTP_POST,
        // handler once file upload finishes
        [&]() {
            server.sendHeader("Connection", "close");
            server.sendHeader("Access-Control-Allow-Origin", "*");
            server.send(200, "text/plain", (Update.hasError()) ? "FAIL" : "OK");
            ESP.restart();
        },
        // handler for upload, get's the sketch bytes,
        // and writes them through the Update object
        [&]() {
            HTTPUpload &upload = server.upload();

            if (upload.status == UPLOAD_FILE_START)
            {
                uint32_t maxSketchSpace = (ESP.getFreeSketchSpace() - 0x1000) & 0xFFFFF000;
                WiFiUDP::stopAll();
                Serial.printf("Update: %s\n", upload.filename.c_str());
                led_rgb_on(COLOR_MAGENTA);
                ota_blink = true;

                //start with max available size
                if (!Update.begin(maxSketchSpace))
                    Update.printError(Serial1);
            }
            else if (upload.status == UPLOAD_FILE_WRITE)
            {
                if (ota_blink)
                {
                    led_rgb_on(COLOR_MAGENTA);
                }
                else
                {
                    led_rgb_off();
                }
                ota_blink = !ota_blink;
                Serial.print(".");
                if (Update.write(upload.buf, upload.currentSize) != upload.currentSize)
                    Update.printError(Serial1);
            }
            else if (upload.status == UPLOAD_FILE_END)
            {
                //true to set the size to the current progress
                if (Update.end(true))
                {
                    Serial.printf("Update Success: %u\nRebooting...\n", upload.totalSize);
                }
                else
                {
                    Update.printError(Serial1);
                }
                led_rgb_off();
            }
            else if (upload.status == UPLOAD_FILE_ABORTED)
            {
                Update.end();
                led_rgb_off();
                Serial.println(F("Update was aborted"));
            }
            delay(0);
        });
}
