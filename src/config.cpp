// module téléinformation client
// rene-d 2020

// **********************************************************************************
// ESP8266 Teleinfo WEB Server configuration Include file
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

#include "config.h"
#include "tic.h"
#include "jsonbuilder.h"
#include <EEPROM.h>
#include <user_interface.h>

#include "emptyserial.h"

// Configuration structure for whole program
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

/* ======================================================================
Function: ResetConfig
Purpose : Set configuration to default values
Input   : -
Output  : -
Comments: -
====================================================================== */
void config_reset()
{
    // Start cleaning all that stuff
    memset(&config, 0, sizeof(Config));

    // Set default Hostname
    sprintf_P(config.host, PSTR("WifInfo-%06X"), ESP.getChipId());
    strcpy_P(config.ota_auth, DEFAULT_OTA_AUTH);
    config.ota_port = DEFAULT_OTA_PORT;

    // Add other init default config here

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
    strcpy_P(config.httpReq.host, CFG_HTTPREQ_DEFAULT_HOST);
    config.httpReq.port = CFG_HTTPREQ_DEFAULT_PORT;
    strcpy_P(config.httpReq.url, CFG_HTTPREQ_DEFAULT_URL);

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
            crc = (crc >> 1) ^ 0xA001;
        else
            crc = (crc >> 1);
    }
    return crc;
}

// fill config structure with data located into eeprom
bool config_read(bool clear_on_error)
{
    uint16_t crc = ~0;
    uint8_t *pconfig = (uint8_t *)&config;
    uint8_t data;

    // For whole size of config structure
    for (size_t i = 0; i < sizeof(Config); ++i)
    {
        // read data
        data = EEPROM.read(i);

        // save into struct
        *pconfig++ = data;

        // calc CRC
        crc = crc16Update(crc, data);
    }

    // CRC Error ?
    if (crc != 0)
    {
        // Clear config if wanted
        if (clear_on_error)
            memset(&config, 0, sizeof(Config));
        return false;
    }

    return true;
}

/* ======================================================================
Function: config_save
Purpose : save config structure values into eeprom
Input 	: -
Output	: true if saved and readback ok
Comments: once saved, config is read again to check the CRC
====================================================================== */
bool config_save(void)
{
    const uint8_t *pconfig;
    bool ret_code;

    // Init pointer
    pconfig = (const uint8_t *)&config;

    // Init CRC
    config.crc = ~0;

    // For whole size of config structure, pre-calculate CRC
    for (size_t i = 0; i < sizeof(Config) - 2; ++i)
        config.crc = crc16Update(config.crc, *pconfig++);

    // Re init pointer
    pconfig = (const uint8_t *)&config;

    // For whole size of config structure, write to EEP
    for (size_t i = 0; i < sizeof(Config); ++i)
        EEPROM.write(i, *pconfig++);

    // Physically save
    EEPROM.commit();

    // Read Again to see if saved ok, but do
    // not clear if error this avoid clearing
    // default config and breaks OTA
    ret_code = config_read(false);

    Serial.println(F("Write config "));

    // return result
    return (ret_code);
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

    Serial.print(F("Config   :"));
    if (config.config & CONFIG_LED_TINFO)
        Serial.print(F(" LED_TINFO"));
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
    Serial.println(config.httpReq.host);
    Serial.print(F("port      : "));
    Serial.println(config.httpReq.port);
    Serial.print(F("url       : "));
    Serial.println(config.httpReq.url);
    Serial.print(F("freq      : "));
    Serial.println(config.httpReq.freq);
    Serial.print(F("notifs    :"));
    if (config.httpReq.trigger_ptec)
        Serial.print(F(" PTEC"));
    if (config.httpReq.trigger_adps)
        Serial.print(F(" ADPS"));
    if (config.httpReq.trigger_seuils)
        Serial.print(F(" seuils"));
    Serial.println();
    Serial.print(F("seuil bas : "));
    Serial.println(config.httpReq.seuil_bas);
    Serial.print(F("seuil haut: "));
    Serial.println(config.httpReq.seuil_haut);

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

    js.append(FPSTR("cfg_led_info"), (config.config & CONFIG_LED_TINFO) ? 1 : 0);

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

    js.append(CFG_FORM_HTTPREQ_HOST, config.httpReq.host);
    js.append(CFG_FORM_HTTPREQ_PORT, config.httpReq.port);
    js.append(CFG_FORM_HTTPREQ_URL, config.httpReq.url);
    js.append(CFG_FORM_HTTPREQ_FREQ, config.httpReq.freq);

    js.append(CFG_FORM_HTTPREQ_TRIGGER_PTEC, config.httpReq.trigger_ptec);
    js.append(CFG_FORM_HTTPREQ_TRIGGER_ADPS, config.httpReq.trigger_adps);
    js.append(CFG_FORM_HTTPREQ_TRIGGER_SEUILS, config.httpReq.trigger_seuils);
    js.append(CFG_FORM_HTTPREQ_SEUIL_BAS, config.httpReq.seuil_bas);
    js.append(CFG_FORM_HTTPREQ_SEUIL_HAUT, config.httpReq.seuil_haut, true);
}

static int validate_int(const String &value, int a, int b, int d)
{
    int v = value.toInt();
    if (a <= v && v <= b)
        return v;
    return d;
}

void config_handle_form(ESP8266WebServer &server)
{
    String response;
    int ret;

    // We validated config ?
    if (server.hasArg("save"))
    {
#ifdef DEBUG
        Serial.println(F("===== Posted configuration"));
        for (int i = 0; i < server.args(); ++i)
            Serial.printf("  %3d  %-20s = %s\n", i, server.argName(i).c_str(), server.arg(i).c_str());
        Serial.println(F("===== Posted configuration"));
#endif

        // WifInfo
        strncpy(config.ssid, server.arg(CFG_FORM_SSID).c_str(), CFG_SSID_LENGTH);
        strncpy(config.psk, server.arg("psk").c_str(), CFG_SSID_LENGTH);
        strncpy(config.host, server.arg("host").c_str(), CFG_HOSTNAME_LENGTH);
        strncpy(config.ap_psk, server.arg("ap_psk").c_str(), CFG_SSID_LENGTH);
        strncpy(config.ota_auth, server.arg("ota_auth").c_str(), CFG_SSID_LENGTH);
        config.ota_port = validate_int(server.arg("ota_port"), 0, 65535, DEFAULT_OTA_PORT);

        config.config = 0;
        if (server.hasArg("cfg_led_tinfo"))
            config.config |= CONFIG_LED_TINFO;

        // Emoncms
        strncpy(config.emoncms.host, server.arg("emon_host").c_str(), CFG_EMON_HOST_LENGTH);
        strncpy(config.emoncms.url, server.arg("emon_url").c_str(), CFG_EMON_URL_LENGTH);
        strncpy(config.emoncms.apikey, server.arg("emon_apikey").c_str(), CFG_EMON_APIKEY_LENGTH);
        config.emoncms.node = validate_int(server.arg("emon_node"), 0, 255, 0);
        config.emoncms.port = validate_int(server.arg("emon_port"), 0, 65535, CFG_EMON_DEFAULT_PORT);
        config.emoncms.freq = validate_int(server.arg("emon_freq"), 0, 86400, 0);

        // jeedom
        strncpy(config.jeedom.host, server.arg(CFG_FORM_JDOM_HOST).c_str(), CFG_JDOM_HOST_LENGTH);
        strncpy(config.jeedom.url, server.arg(CFG_FORM_JDOM_URL).c_str(), CFG_JDOM_URL_LENGTH);
        strncpy(config.jeedom.apikey, server.arg(CFG_FORM_JDOM_KEY).c_str(), CFG_JDOM_APIKEY_LENGTH);
        strncpy(config.jeedom.adco, server.arg(CFG_FORM_JDOM_ADCO).c_str(), CFG_JDOM_ADCO_LENGTH);
        config.jeedom.port = validate_int(server.arg(CFG_FORM_JDOM_PORT), 0, 65535, CFG_JDOM_DEFAULT_PORT);
        config.jeedom.freq = validate_int(server.arg(CFG_FORM_JDOM_FREQ), 0, 86400, 0);

        // HTTP Request
        strncpy(config.httpReq.host, server.arg(CFG_FORM_HTTPREQ_HOST).c_str(), CFG_HTTPREQ_HOST_LENGTH);
        strncpy(config.httpReq.url, server.arg(CFG_FORM_HTTPREQ_URL).c_str(), CFG_HTTPREQ_URL_LENGTH);
        config.httpReq.port = validate_int(server.arg(CFG_FORM_HTTPREQ_PORT), 0, 65535, CFG_HTTPREQ_DEFAULT_PORT);
        config.httpReq.freq = validate_int(server.arg(CFG_FORM_HTTPREQ_FREQ), 0, 86400, 0);

        config.httpReq.trigger_ptec = server.hasArg(CFG_FORM_HTTPREQ_TRIGGER_PTEC);
        config.httpReq.trigger_adps = server.hasArg(CFG_FORM_HTTPREQ_TRIGGER_ADPS);
        config.httpReq.trigger_seuils = server.hasArg(CFG_FORM_HTTPREQ_TRIGGER_SEUILS);
        config.httpReq.seuil_bas = validate_int(server.arg(CFG_FORM_HTTPREQ_SEUIL_BAS), 0, 20000, 0);
        config.httpReq.seuil_haut = validate_int(server.arg(CFG_FORM_HTTPREQ_SEUIL_HAUT), 0, 20000, 0);

        if (config_save())
        {
            ret = 200;
            response = F("OK");
        }
        else
        {
            ret = 412;
            response = F("Unable to save configuration");
        }

        config_show();
    }
    else
    {
        ret = 400;
        response = F("Missing Form Field");
    }

    Serial.printf_P(PSTR("Sending response %d %s\n"), ret, response.c_str());

    server.send(ret, "text/plain", response);

    // reprogramme les timers de notification
    tic_make_timers();
}
