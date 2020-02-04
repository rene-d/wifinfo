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
#include "debug.h"
#include <EEPROM.h>

// Configuration structure for whole program
_Config config;

void config_setup()
{
    // Our configuration is stored into EEPROM
    //EEPROM.begin(sizeof(_Config));
    EEPROM.begin(1024);

    DebugF("Config size=");
    Debug(sizeof(_Config));
    DebugF(" (emoncms=");
    Debug(sizeof(_emoncms));
    DebugF("  jeedom=");
    Debug(sizeof(_jeedom));
    DebugF("  http request=");
    Debug(sizeof(_httpRequest));
    Debugln(" )");
    //  Debugflush();

    // Read Configuration from EEP
    if (config_read())
    {
        DebuglnF("Good CRC, not set! From now, we can use EEPROM config !");
    }
    else
    {
        // Reset Configuration
        config_reset();

        // save back
        config_save();

        // Indicate the error in global flags
        config.config |= CFG_BAD_CRC;

        DebuglnF("Reset to default");
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
    char buff[128];

    // default to 16 bytes per row
    if (bytesPerRow == 0)
        bytesPerRow = 16;

    Debugln();

    // loop thru EEP address
    for (i = 0; i < size; i++)
    {
        // First byte of the row ?
        if (j == 0)
        {
            // Display Address
            sprintf_P(buff, PSTR("%04X : "), i);
            Debug(buff);
        }

        // write byte in hex form
        sprintf_P(buff, PSTR("%02X "), EEPROM.read(i));
        Debug(buff);

        // Last byte of the row ?
        // start a new line
        if (++j >= bytesPerRow)
        {
            j = 0;
            Debugln();
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

    Debugln(F("Write config "));

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
    Debugln("");
    DebuglnF("===== Wifi");
    DebugF("ssid     :");
    Debugln(config.ssid);
    DebugF("psk      :");
    Debugln(config.psk);
    DebugF("host     :");
    Debugln(config.host);
    DebuglnF("===== Avancé");
    DebugF("ap_psk   :");
    Debugln(config.ap_psk);
    DebugF("OTA auth :");
    Debugln(config.ota_auth);
    DebugF("OTA port :");
    Debugln(config.ota_port);

    DebugF("Config   :");
    if (config.config & CFG_RGB_LED)
        DebugF(" RGB");
    if (config.config & CFG_DEBUG)
        DebugF(" DEBUG");
    if (config.config & CFG_LCD)
        DebugF(" LCD");
    Debugln("");

    DebuglnF("===== Emoncms");
    DebugF("host     :");
    Debugln(config.emoncms.host);
    DebugF("port     :");
    Debugln((int)config.emoncms.port);
    DebugF("url      :");
    Debugln(config.emoncms.url);
    DebugF("key      :");
    Debugln(config.emoncms.apikey);
    DebugF("node     :");
    Debugln(config.emoncms.node);
    DebugF("freq     :");
    Debugln(config.emoncms.freq);

    DebuglnF("===== Jeedom");
    DebugF("host     :");
    Debugln(config.jeedom.host);
    DebugF("port     :");
    Debugln(config.jeedom.port);
    DebugF("url      :");
    Debugln(config.jeedom.url);
    DebugF("key      :");
    Debugln(config.jeedom.apikey);
    DebugF("compteur :");
    Debugln(config.jeedom.adco);
    DebugF("freq     :");
    Debugln(config.jeedom.freq);

    DebuglnF("===== HTTP request");
    DebugF("host     :");
    Debugln(config.httpReq.host);
    DebugF("port     :");
    Debugln(config.httpReq.port);
    DebugF("path     :");
    Debugln(config.httpReq.path);
    DebugF("freq     :");
    Debugln(config.httpReq.freq);

    Serial.flush();
}

const char FP_QCQ[] PROGMEM = "\":\"";
const char FP_QCNL[] PROGMEM = "\",\"";

/* ======================================================================
Function: getConfigJSONData
Purpose : Return JSON string containing configuration data
Input   : Response String
Output  : -
Comments: -
====================================================================== */
void config_get_json(String &r)
{
    // Json start
    r = FPSTR("{");

    r += "\"";
    r += CFG_FORM_SSID;
    r += FPSTR(FP_QCQ);
    r += config.ssid;
    r += FPSTR(FP_QCNL);
    r += CFG_FORM_PSK;
    r += FPSTR(FP_QCQ);
    r += config.psk;
    r += FPSTR(FP_QCNL);
    r += CFG_FORM_HOST;
    r += FPSTR(FP_QCQ);
    r += config.host;
    r += FPSTR(FP_QCNL);
    r += CFG_FORM_AP_PSK;
    r += FPSTR(FP_QCQ);
    r += config.ap_psk;
    r += FPSTR(FP_QCNL);
    r += CFG_FORM_EMON_HOST;
    r += FPSTR(FP_QCQ);
    r += config.emoncms.host;
    r += FPSTR(FP_QCNL);
    r += CFG_FORM_EMON_PORT;
    r += FPSTR(FP_QCQ);
    r += config.emoncms.port;
    r += FPSTR(FP_QCNL);
    r += CFG_FORM_EMON_URL;
    r += FPSTR(FP_QCQ);
    r += config.emoncms.url;
    r += FPSTR(FP_QCNL);
    r += CFG_FORM_EMON_KEY;
    r += FPSTR(FP_QCQ);
    r += config.emoncms.apikey;
    r += FPSTR(FP_QCNL);
    r += CFG_FORM_EMON_NODE;
    r += FPSTR(FP_QCQ);
    r += config.emoncms.node;
    r += FPSTR(FP_QCNL);
    r += CFG_FORM_EMON_FREQ;
    r += FPSTR(FP_QCQ);
    r += config.emoncms.freq;
    r += FPSTR(FP_QCNL);
    r += CFG_FORM_OTA_AUTH;
    r += FPSTR(FP_QCQ);
    r += config.ota_auth;
    r += FPSTR(FP_QCNL);
    r += CFG_FORM_OTA_PORT;
    r += FPSTR(FP_QCQ);
    r += config.ota_port;
    r += FPSTR(FP_QCNL);
    r += CFG_FORM_JDOM_HOST;
    r += FPSTR(FP_QCQ);
    r += config.jeedom.host;
    r += FPSTR(FP_QCNL);
    r += CFG_FORM_JDOM_PORT;
    r += FPSTR(FP_QCQ);
    r += config.jeedom.port;
    r += FPSTR(FP_QCNL);
    r += CFG_FORM_JDOM_URL;
    r += FPSTR(FP_QCQ);
    r += config.jeedom.url;
    r += FPSTR(FP_QCNL);
    r += CFG_FORM_JDOM_KEY;
    r += FPSTR(FP_QCQ);
    r += config.jeedom.apikey;
    r += FPSTR(FP_QCNL);
    r += CFG_FORM_JDOM_ADCO;
    r += FPSTR(FP_QCQ);
    r += config.jeedom.adco;
    r += FPSTR(FP_QCNL);
    r += CFG_FORM_JDOM_FREQ;
    r += FPSTR(FP_QCQ);
    r += config.jeedom.freq;
    r += FPSTR(FP_QCNL);
    r += CFG_FORM_HTTPREQ_HOST;
    r += FPSTR(FP_QCQ);
    r += config.httpReq.host;
    r += FPSTR(FP_QCNL);
    r += CFG_FORM_HTTPREQ_PORT;
    r += FPSTR(FP_QCQ);
    r += config.httpReq.port;
    r += FPSTR(FP_QCNL);
    r += CFG_FORM_HTTPREQ_PATH;
    r += FPSTR(FP_QCQ);
    r += config.httpReq.path;
    r += FPSTR(FP_QCNL);
    r += CFG_FORM_HTTPREQ_FREQ;
    r += FPSTR(FP_QCQ);
    r += config.httpReq.freq;
    r += F("\"");

    // Json end
    r += FPSTR("}");
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
        DebuglnF("===== Posted configuration");

        // WifInfo
        strncpy(config.ssid, server.arg("ssid").c_str(), CFG_SSID_SIZE);
        strncpy(config.psk, server.arg("psk").c_str(), CFG_PSK_SIZE);
        strncpy(config.host, server.arg("host").c_str(), CFG_HOSTNAME_SIZE);
        strncpy(config.ap_psk, server.arg("ap_psk").c_str(), CFG_PSK_SIZE);
        strncpy(config.ota_auth, server.arg("ota_auth").c_str(), CFG_PSK_SIZE);
        config.ota_port = validate_int(server.arg("ota_port"), 0, 65535, DEFAULT_OTA_PORT);

        // Emoncms
        strncpy(config.emoncms.host, server.arg("emon_host").c_str(), CFG_EMON_HOST_SIZE);
        strncpy(config.emoncms.url, server.arg("emon_url").c_str(), CFG_EMON_URL_SIZE);
        strncpy(config.emoncms.apikey, server.arg("emon_apikey").c_str(), CFG_EMON_APIKEY_SIZE);
        config.emoncms.node = validate_int(server.arg("emon_node"), 0, 255, 0);
        config.emoncms.port = validate_int(server.arg("emon_port"), 0, 65535, CFG_EMON_DEFAULT_PORT);
        config.emoncms.freq = validate_int(server.arg("emon_freq"), 0, 86400, 0);

        // jeedom
        strncpy(config.jeedom.host, server.arg("jdom_host").c_str(), CFG_JDOM_HOST_SIZE);
        strncpy(config.jeedom.url, server.arg("jdom_url").c_str(), CFG_JDOM_URL_SIZE);
        strncpy(config.jeedom.apikey, server.arg("jdom_apikey").c_str(), CFG_JDOM_APIKEY_SIZE);
        strncpy(config.jeedom.adco, server.arg("jdom_adco").c_str(), CFG_JDOM_ADCO_SIZE);
        config.jeedom.port = validate_int(server.arg("jdom_port"), 0, 65535, CFG_JDOM_DEFAULT_PORT);
        config.jeedom.freq = validate_int(server.arg("jdom_freq"), 0, 86400, 0);

        // HTTP Request
        strncpy(config.httpReq.host, server.arg("httpreq_host").c_str(), CFG_HTTPREQ_HOST_SIZE);
        strncpy(config.httpReq.path, server.arg("httpreq_path").c_str(), CFG_HTTPREQ_PATH_SIZE);
        config.httpReq.port = validate_int(server.arg("httpreq_port"), 0, 65535, CFG_HTTPREQ_DEFAULT_PORT);
        config.httpReq.freq = validate_int(server.arg("httpreq_freq"), 0, 86400, 0);

        if (config_save())
        {
            ret = 200;
            response = "OK";
        }
        else
        {
            ret = 412;
            response = "Unable to save configuration";
        }

        config_show();
    }
    else
    {
        ret = 400;
        response = "Missing Form Field";
    }

    Debugf_P(PSTR("Sending response %d %s\n"), ret, response.c_str());

    server.send(ret, "text/plain", response);

    // reprogramme les timers de notification
    tic_make_timers();
}
