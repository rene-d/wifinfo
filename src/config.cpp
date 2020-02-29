// module téléinformation client
// rene-d 2020

// **********************************************************************************
// ESP8266 Teleinfo WEB Server
// **********************************************************************************
// Creative Commons Attrib Share-Alike License
// You are free to use/extend this library but please abide with the CC-BY-SA license:
// Attribution-NonCommercial-ShareAlike 4.0 International License
// http://creativecommons.org/licenses/by-nc-sa/4.0/
//
// For any explanation about teleinfo ou use , see my blog
// http://hallard.me/category/tinfo
//
// This program works with the Wifinfo board
// see schematic here https://github.com/hallard/teleinfo/tree/master/Wifinfo
//
// Written by Charles-Henri Hallard (http://hallard.me)
//
// History : V1.00 2015-06-14 - First release
//
// All text above must be included in any redistribution.
//
// **********************************************************************************

#include "settings.h"
#include "config.h"
#include "jsonbuilder.h"
#include "strncpy_s.h"
#include "tic.h"
#include <EEPROM.h>
#include <user_interface.h>

#include "emptyserial.h"

// Configuration object for whole program
Config config;


void config_setup()
{
    // Our configuration is stored into EEPROM
    EEPROM.begin(sizeof(Config));

    // Read Configuration from EEP
    if (config_read())
    {
        Serial.println(F("Good CRC, not set! From now, we can use EEPROM config !"));
    }
    else
    {
        // Reset Configuration
        config_reset();

        // save back
        config_save();

        Serial.println(F("Reset to default"));
    }

}

static void config_securize_cstrings()
{
    // ajoute des \0 pour plus de sécurité
    config.ssid[CFG_SSID_LENGTH] = 0;
    config.psk[CFG_PSK_LENGTH] = 0;
    config.host[CFG_HOSTNAME_LENGTH] = 0;
    config.ap_psk[CFG_PSK_LENGTH] = 0;
    config.ota_auth[CFG_PSK_LENGTH] = 0;

    config.emoncms.host[CFG_EMON_HOST_LENGTH] = 0;
    config.emoncms.apikey[CFG_EMON_KEY_LENGTH] = 0;
    config.emoncms.url[CFG_EMON_URL_LENGTH] = 0;

    config.jeedom.host[CFG_JDOM_HOST_LENGTH] = 0;
    config.jeedom.apikey[CFG_JDOM_KEY_LENGTH] = 0;
    config.jeedom.url[CFG_JDOM_URL_LENGTH] = 0;
    config.jeedom.adco[CFG_JDOM_ADCO_LENGTH] = 0;

    config.httpreq.host[CFG_HTTPREQ_HOST_LENGTH] = 0;
    config.httpreq.url[CFG_HTTPREQ_URL_LENGTH] = 0;
}

// Set configuration to default values
void config_reset()
{
    // Start cleaning all that stuff
    memset(&config, 0, sizeof(Config));

    // Set default Hostname
    sprintf_P(config.host, PSTR("WifInfo-%06X"), ESP.getChipId());
    strcpy_P(config.ota_auth, DEFAULT_OTA_AUTH);
    config.ota_port = DEFAULT_OTA_PORT;
    config.sse_freq = 10;

    // Emoncms
    strcpy_P(config.emoncms.host, CFG_EMON_DEFAULT_HOST);
    config.emoncms.port = CFG_EMON_DEFAULT_PORT;
    strcpy_P(config.emoncms.url, CFG_EMON_DEFAULT_URL);

    // Jeedom
    strcpy_P(config.jeedom.host, CFG_JDOM_DEFAULT_HOST);
    config.jeedom.port = CFG_JDOM_DEFAULT_PORT;
    strcpy_P(config.jeedom.url, CFG_JDOM_DEFAULT_URL);
    strcpy_P(config.jeedom.adco, CFG_JDOM_DEFAULT_ADCO);

    // HTTP Request
    strcpy_P(config.httpreq.host, CFG_HTTPREQ_DEFAULT_HOST);
    config.httpreq.port = CFG_HTTPREQ_DEFAULT_PORT;
    strcpy_P(config.httpreq.url, CFG_HTTPREQ_DEFAULT_URL);

    // save back
    config_save();
}

static uint16_t crc16Update(uint16_t crc, uint8_t a)
{
    int i;
    crc ^= a;
    for (i = 0; i < 8; ++i)
    {
        if (crc & 1)
        {
            crc = (crc >> 1) ^ 0xA001;
        }
        else
        {
            crc = (crc >> 1);
        }
    }
    return crc;
}

// fill config structure with data located into eeprom
bool config_read(bool clear_on_error)
{
    uint16_t crc = ~0;
    uint8_t *pconfig = (uint8_t *)&config;

    // For whole size of config structure
    for (size_t i = 0; i < sizeof(Config); ++i)
    {
        // read data
        uint8_t data = EEPROM.read(i);

        // save into struct
        *pconfig++ = data;

        // calc CRC
        crc = crc16Update(crc, data);
    }

    config_securize_cstrings();

    // CRC Error ?
    if (crc != 0)
    {
        // Clear config if wanted
        if (clear_on_error)
        {
            memset(&config, 0, sizeof(Config));
        }
        return false;
    }

    return true;
}

// save config structure values into eeprom
bool config_save()
{
    const uint8_t *pconfig;
    bool ret_code;

    // Init pointer
    pconfig = (const uint8_t *)&config;

    // Init CRC
    config.crc = ~0;

    // For whole size of config structure, pre-calculate CRC
    for (size_t i = 0; i < sizeof(Config) - 2; ++i)
    {
        config.crc = crc16Update(config.crc, *pconfig++);
    }

    // Re init pointer
    pconfig = (const uint8_t *)&config;

    // For whole size of config structure, write to EEP
    for (size_t i = 0; i < sizeof(Config); ++i)
    {
        EEPROM.write(i, *pconfig++);
    }
    // Physically save
    EEPROM.commit();

    // Read Again to see if saved ok, but do
    // not clear if error this avoid clearing
    // default config and breaks OTA
    ret_code = config_read(false);

    Serial.println(F("Write config "));

    // return result
    return ret_code;
}

// print configuration
void config_show()
{
    Serial.println();
    Serial.println(F("===== Wi-Fi"));
    Serial.print(F("ssid     :"));
    Serial.println(config.ssid);
    Serial.print(F("psk      :"));
    Serial.println(config.psk);
    Serial.print(F("host     :"));
    Serial.println(config.host);
    Serial.println(F("===== Advanced"));
    Serial.print(F("ap_psk   :"));
    Serial.println(config.ap_psk);
    Serial.print(F("OTA auth :"));
    Serial.println(config.ota_auth);
    Serial.print(F("OTA port :"));
    Serial.println(config.ota_port);

    Serial.print(F("SSE freq :"));
    Serial.println(config.sse_freq);

    Serial.print(F("Config   :"));
    if (config.options & OPTION_LED_TINFO)
    {
        Serial.print(F(" LED_TINFO"));
    }
    Serial.println();

    Serial.println(F("===== Emoncms"));
    Serial.print(F("host     :"));
    Serial.println(config.emoncms.host);
    Serial.print(F("port     :"));
    Serial.println(config.emoncms.port);
    Serial.print(F("url      :"));
    Serial.println(config.emoncms.url);
    Serial.print(F("key      :"));
    Serial.println(config.emoncms.apikey);
    Serial.print(F("node     :"));
    Serial.println(config.emoncms.node);
    Serial.print(F("freq     :"));
    Serial.println(config.emoncms.freq);

    Serial.println(F("===== Jeedom"));
    Serial.print(F("host     :"));
    Serial.println(config.jeedom.host);
    Serial.print(F("port     :"));
    Serial.println(config.jeedom.port);
    Serial.print(F("url      :"));
    Serial.println(config.jeedom.url);
    Serial.print(F("key      :"));
    Serial.println(config.jeedom.apikey);
    Serial.print(F("compteur :"));
    Serial.println(config.jeedom.adco);
    Serial.print(F("freq     :"));
    Serial.println(config.jeedom.freq);

    Serial.println(F("===== HTTP request"));
    Serial.print(F("host      : "));
    Serial.println(config.httpreq.host);
    Serial.print(F("port      : "));
    Serial.println(config.httpreq.port);
    Serial.print(F("url       : "));
    Serial.print(F("method    : "));
    if (config.httpreq.use_post)
    {
        Serial.println(F("POST"));
    }
    else
    {
        Serial.println(F("GET"));
    }
    Serial.println(config.httpreq.url);
    Serial.print(F("freq      : "));
    Serial.println(config.httpreq.freq);
    Serial.print(F("notifs    :"));
    if (config.httpreq.trigger_ptec)
    {
        Serial.print(F(" PTEC"));
    }
    if (config.httpreq.trigger_adps)
    {
        Serial.print(F(" ADPS"));
    }
    if (config.httpreq.trigger_seuils)
    {
        Serial.print(F(" seuils"));
    }
    Serial.println();
    Serial.print(F("seuil bas : "));
    Serial.println(config.httpreq.seuil_bas);
    Serial.print(F("seuil haut: "));
    Serial.println(config.httpreq.seuil_haut);

    Serial.flush();
}

// Return JSON string containing configuration data
void config_get_json(String &r)
{
    JSONBuilder js(r, 1024);

    js.append(CFG_FORM_SSID, config.ssid);
    js.append(CFG_FORM_PSK, config.psk);
    js.append(CFG_FORM_HOST, config.host);
    js.append(CFG_FORM_AP_PSK, config.ap_psk);

    js.append(CFG_FORM_OTA_AUTH, config.ota_auth);
    js.append(CFG_FORM_OTA_PORT, config.ota_port);

    js.append(CFG_FORM_SSE_FREQ, config.sse_freq);
    js.append(CFG_LED_TINFO, (config.options & OPTION_LED_TINFO) ? 1 : 0);

    js.append(CFG_FORM_EMON_HOST, config.emoncms.host);
    js.append(CFG_FORM_EMON_PORT, config.emoncms.port);
    js.append(CFG_FORM_EMON_URL, config.emoncms.url);
    js.append(CFG_FORM_EMON_KEY, config.emoncms.apikey);
    js.append(CFG_FORM_EMON_NODE, config.emoncms.node);
    js.append(CFG_FORM_EMON_FREQ, config.emoncms.freq);

    js.append(CFG_FORM_JDOM_HOST, config.jeedom.host);
    js.append(CFG_FORM_JDOM_PORT, config.jeedom.port);
    js.append(CFG_FORM_JDOM_URL, config.jeedom.url);
    js.append(CFG_FORM_JDOM_KEY, config.jeedom.apikey);
    js.append(CFG_FORM_JDOM_ADCO, config.jeedom.adco);
    js.append(CFG_FORM_JDOM_FREQ, config.jeedom.freq);

    js.append(CFG_FORM_HTTPREQ_HOST, config.httpreq.host);
    js.append(CFG_FORM_HTTPREQ_PORT, config.httpreq.port);
    js.append(CFG_FORM_HTTPREQ_URL, config.httpreq.url);
    js.append(CFG_FORM_HTTPREQ_USE_POST, config.httpreq.use_post);

    js.append(CFG_FORM_HTTPREQ_FREQ, config.httpreq.freq);
    js.append(CFG_FORM_HTTPREQ_TRIGGER_PTEC, config.httpreq.trigger_ptec);
    js.append(CFG_FORM_HTTPREQ_TRIGGER_ADPS, config.httpreq.trigger_adps);
    js.append(CFG_FORM_HTTPREQ_TRIGGER_SEUILS, config.httpreq.trigger_seuils);
    js.append(CFG_FORM_HTTPREQ_SEUIL_BAS, config.httpreq.seuil_bas);
    js.append(CFG_FORM_HTTPREQ_SEUIL_HAUT, config.httpreq.seuil_haut, true);
}

static int validate_int(const String &value, int a, int b, int d)
{
    int v = value.toInt();
    if (a <= v && v <= b)
    {
        return v;
    }
    return d;
}

void config_handle_form(ESP8266WebServer &server)
{
    const char *response;
    int ret;

    // We validated config ?
    if (server.hasArg("save"))
    {
#ifdef ENABLE_DEBUG
        Serial.println(F("===== Posted configuration"));
        for (int i = 0; i < server.args(); ++i)
            Serial.printf("  %3d  %-20s = %s\n", i, server.argName(i).c_str(), server.arg(i).c_str());
        Serial.println(F("===== Posted configuration"));
#endif

        // Wi-Fi et avancé
        strncpy_s(config.ssid, server.arg(CFG_FORM_SSID), CFG_SSID_LENGTH);
        strncpy_s(config.psk, server.arg(CFG_FORM_PSK), CFG_PSK_LENGTH);
        strncpy_s(config.host, server.arg(CFG_FORM_HOST), CFG_HOSTNAME_LENGTH);
        strncpy_s(config.ap_psk, server.arg(CFG_FORM_AP_PSK), CFG_PSK_LENGTH);
        strncpy_s(config.ota_auth, server.arg(CFG_FORM_OTA_AUTH), CFG_PSK_LENGTH);
        config.ota_port = validate_int(server.arg(CFG_FORM_OTA_PORT), 0, 65535, DEFAULT_OTA_PORT);

        config.sse_freq = validate_int(server.arg(CFG_FORM_SSE_FREQ), 0, 360, 0);

        config.options = 0;
        if (server.hasArg(CFG_LED_TINFO))
            config.options |= OPTION_LED_TINFO;

        // Emoncms
        strncpy_s(config.emoncms.host, server.arg(CFG_FORM_EMON_HOST), CFG_EMON_HOST_LENGTH);
        config.emoncms.port = validate_int(server.arg(CFG_FORM_EMON_PORT), 0, 65535, CFG_EMON_DEFAULT_PORT);
        strncpy_s(config.emoncms.url, server.arg(CFG_FORM_EMON_URL), CFG_EMON_URL_LENGTH);
        strncpy_s(config.emoncms.apikey, server.arg(CFG_FORM_EMON_KEY), CFG_EMON_KEY_LENGTH);
        config.emoncms.node = validate_int(server.arg(CFG_FORM_EMON_NODE), 0, 255, 0);
        config.emoncms.freq = validate_int(server.arg(CFG_FORM_EMON_FREQ), 0, 86400, 0);

        // jeedom
        strncpy_s(config.jeedom.host, server.arg(CFG_FORM_JDOM_HOST), CFG_JDOM_HOST_LENGTH);
        config.jeedom.port = validate_int(server.arg(CFG_FORM_JDOM_PORT), 0, 65535, CFG_JDOM_DEFAULT_PORT);
        strncpy_s(config.jeedom.url, server.arg(CFG_FORM_JDOM_URL), CFG_JDOM_URL_LENGTH);
        strncpy_s(config.jeedom.apikey, server.arg(CFG_FORM_JDOM_KEY), CFG_JDOM_KEY_LENGTH);
        strncpy_s(config.jeedom.adco, server.arg(CFG_FORM_JDOM_ADCO), CFG_JDOM_ADCO_LENGTH);
        config.jeedom.freq = validate_int(server.arg(CFG_FORM_JDOM_FREQ), 0, 86400, 0);

        // HTTP Request
        strncpy_s(config.httpreq.host, server.arg(CFG_FORM_HTTPREQ_HOST), CFG_HTTPREQ_HOST_LENGTH);
        config.httpreq.port = validate_int(server.arg(CFG_FORM_HTTPREQ_PORT), 0, 65535, CFG_HTTPREQ_DEFAULT_PORT);
        strncpy_s(config.httpreq.url, server.arg(CFG_FORM_HTTPREQ_URL), CFG_HTTPREQ_URL_LENGTH);
        config.httpreq.use_post = server.hasArg(CFG_FORM_HTTPREQ_USE_POST);

        config.httpreq.freq = validate_int(server.arg(CFG_FORM_HTTPREQ_FREQ), 0, 86400, 0);
        config.httpreq.trigger_adps = server.hasArg(CFG_FORM_HTTPREQ_TRIGGER_ADPS);
        config.httpreq.trigger_ptec = server.hasArg(CFG_FORM_HTTPREQ_TRIGGER_PTEC);
        config.httpreq.trigger_seuils = server.hasArg(CFG_FORM_HTTPREQ_TRIGGER_SEUILS);
        config.httpreq.seuil_bas = validate_int(server.arg(CFG_FORM_HTTPREQ_SEUIL_BAS), 0, 20000, 0);
        config.httpreq.seuil_haut = validate_int(server.arg(CFG_FORM_HTTPREQ_SEUIL_HAUT), 0, 20000, 0);

        if (config_save())
        {
            ret = 200;
            response = PSTR("OK");
        }
        else
        {
            ret = 412;
            response = PSTR("Unable to save configuration");
        }

        config_show();
    }
    else
    {
        ret = 400;
        response = PSTR("Missing Form Field");
    }

    Serial.printf_P(PSTR("Sending response %d %s\n"), ret, response);

    server.send(ret, "text/plain", response);

    // reprogramme les timers de notification
    tic_make_timers();
}
