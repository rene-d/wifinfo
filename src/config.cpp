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

// Configuration structure for whole program
_Config config;

void config_setup()
{
    // Our configuration is stored into EEPROM
    EEPROM.begin(sizeof(_Config));

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

        // Indicate the error in global flags
        config.config |= CFG_BAD_CRC;

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
    memset(&config, 0, sizeof(_Config));

    // Set default Hostname
    sprintf_P(config.host, PSTR("WifInfo-%06X"), ESP.getChipId());
    strcpy_P(config.ota_auth, PSTR(DEFAULT_OTA_AUTH));
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
    //strcpy_P(config.jeedom.adco, CFG_JDOM_DEFAULT_ADCO);

    // HTTP Request
    strcpy_P(config.httpReq.host, CFG_HTTPREQ_DEFAULT_HOST);
    config.httpReq.port = CFG_HTTPREQ_DEFAULT_PORT;
    strcpy_P(config.httpReq.path, CFG_HTTPREQ_DEFAULT_PATH);

    config.config |= CFG_RGB_LED;

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

/* ======================================================================
Function: eeprom_dump
Purpose : dump eeprom value to serial
Input 	: -
Output	: -
Comments: -
====================================================================== */
void eeprom_dump(uint8_t bytesPerRow, size_t size)
{
    size_t i;
    size_t j = 0;

    // default to 16 bytes per row
    if (bytesPerRow == 0)
        bytesPerRow = 16;

    Serial.println();

    // loop thru EEP address
    for (i = 0; i < size; i++)
    {
        // First byte of the row ?
        if (j == 0)
        {
            // Display Address
            Serial.printf_P(PSTR("%04X : "), i);
        }

        // write byte in hex form
        Serial.printf_P(PSTR("%02X "), EEPROM.read(i));

        // Last byte of the row ?
        // start a new line
        if (++j >= bytesPerRow)
        {
            j = 0;
            Serial.println();
        }
    }
}

/* ======================================================================
Function: config_read
Purpose : fill config structure with data located into eeprom
Input 	: true if we need to clear actual struc in case of error
Output	: true if config found and crc ok, false otherwise
Comments: -
====================================================================== */
bool config_read(bool clear_on_error)
{
    uint16_t crc = ~0;
    uint8_t *pconfig = (uint8_t *)&config;
    uint8_t data;

    // For whole size of config structure
    for (size_t i = 0; i < sizeof(_Config); ++i)
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
            memset(&config, 0, sizeof(_Config));
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
    for (size_t i = 0; i < sizeof(_Config) - 2; ++i)
        config.crc = crc16Update(config.crc, *pconfig++);

    // Re init pointer
    pconfig = (const uint8_t *)&config;

    // For whole size of config structure, write to EEP
    for (size_t i = 0; i < sizeof(_Config); ++i)
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

/* ======================================================================
Function: config_show
Purpose : display configuration
Input 	: -
Output	: -
Comments: -
====================================================================== */
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
    Serial.print("ap_psk   :");
    Serial.println(config.ap_psk);
    Serial.print(F("OTA auth :"));
    Serial.println(config.ota_auth);
    Serial.print("OTA port :");
    Serial.println(config.ota_port);

    Serial.print("Config   :");
    if (config.config & CFG_RGB_LED)
        Serial.print(" RGB");
    if (config.config & CFG_DEBUG)
        Serial.print(" DEBUG");
    Serial.println("");

    Serial.println(F("===== Emoncms"));
    Serial.print("host     :");
    Serial.println(config.emoncms.host);
    Serial.print("port     :");
    Serial.println((int)config.emoncms.port);
    Serial.print("url      :");
    Serial.println(config.emoncms.url);
    Serial.print("key      :");
    Serial.println(config.emoncms.apikey);
    Serial.print("node     :");
    Serial.println(config.emoncms.node);
    Serial.print("freq     :");
    Serial.println(config.emoncms.freq);

    Serial.println("===== Jeedom");
    Serial.print("host     :");
    Serial.println(config.jeedom.host);
    Serial.print("port     :");
    Serial.println(config.jeedom.port);
    Serial.print("url      :");
    Serial.println(config.jeedom.url);
    Serial.print("key      :");
    Serial.println(config.jeedom.apikey);
    Serial.print("compteur :");
    Serial.println(config.jeedom.adco);
    Serial.print("freq     :");
    Serial.println(config.jeedom.freq);

    Serial.println("===== HTTP request");
    Serial.print("host     :");
    Serial.println(config.httpReq.host);
    Serial.print("port     :");
    Serial.println(config.httpReq.port);
    Serial.print("path     :");
    Serial.println(config.httpReq.path);
    Serial.print("freq     :");
    Serial.println(config.httpReq.freq);

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
    js.append(CFG_FORM_HTTPREQ_PATH, config.httpReq.path);
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
        Serial.println("===== Posted configuration");

        // WifInfo
        strncpy(config.ssid, server.arg(CFG_FORM_SSID).c_str(), CFG_SSID_SIZE - 1);
        strncpy(config.psk, server.arg("psk").c_str(), CFG_PSK_SIZE - 1);
        strncpy(config.host, server.arg("host").c_str(), CFG_HOSTNAME_SIZE - 1);
        strncpy(config.ap_psk, server.arg("ap_psk").c_str(), CFG_PSK_SIZE - 1);
        strncpy(config.ota_auth, server.arg("ota_auth").c_str(), CFG_PSK_SIZE - 1);
        config.ota_port = validate_int(server.arg("ota_port"), 0, 65535, DEFAULT_OTA_PORT);

        // Emoncms
        strncpy(config.emoncms.host, server.arg("emon_host").c_str(), CFG_EMON_HOST_SIZE - 1);
        strncpy(config.emoncms.url, server.arg("emon_url").c_str(), CFG_EMON_URL_SIZE - 1);
        strncpy(config.emoncms.apikey, server.arg("emon_apikey").c_str(), CFG_EMON_APIKEY_SIZE - 1);
        config.emoncms.node = validate_int(server.arg("emon_node"), 0, 255, 0);
        config.emoncms.port = validate_int(server.arg("emon_port"), 0, 65535, CFG_EMON_DEFAULT_PORT);
        config.emoncms.freq = validate_int(server.arg("emon_freq"), 0, 86400, 0);

        // jeedom
        strncpy(config.jeedom.host, server.arg("jdom_host").c_str(), CFG_JDOM_HOST_SIZE - 1);
        strncpy(config.jeedom.url, server.arg("jdom_url").c_str(), CFG_JDOM_URL_SIZE - 1);
        strncpy(config.jeedom.apikey, server.arg("jdom_apikey").c_str(), CFG_JDOM_APIKEY_SIZE - 1);
        strncpy(config.jeedom.adco, server.arg("jdom_adco").c_str(), CFG_JDOM_ADCO_SIZE - 1);
        config.jeedom.port = validate_int(server.arg("jdom_port"), 0, 65535, CFG_JDOM_DEFAULT_PORT);
        config.jeedom.freq = validate_int(server.arg("jdom_freq"), 0, 86400, 0);

        // HTTP Request
        strncpy(config.httpReq.host, server.arg("httpreq_host").c_str(), CFG_HTTPREQ_HOST_SIZE - 1);
        strncpy(config.httpReq.path, server.arg("httpreq_path").c_str(), CFG_HTTPREQ_PATH_SIZE - 1);
        config.httpReq.port = validate_int(server.arg("httpreq_port"), 0, 65535, CFG_HTTPREQ_DEFAULT_PORT);
        config.httpReq.freq = validate_int(server.arg("httpreq_freq"), 0, 86400, 0);

        config.httpReq.trigger_seuils = validate_int(server.arg(CFG_FORM_HTTPREQ_TRIGGER_PTEC), 0, 1, 0);
        config.httpReq.trigger_seuils = validate_int(server.arg(CFG_FORM_HTTPREQ_TRIGGER_ADPS), 0, 1, 0);
        config.httpReq.trigger_seuils = validate_int(server.arg(CFG_FORM_HTTPREQ_TRIGGER_SEUILS), 0, 1, 0);
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
