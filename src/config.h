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
#pragma once

#include <Arduino.h>
#include <ESP8266WebServer.h>
#include <inttypes.h>

#define CFG_SSID_LENGTH 32
#define CFG_PSK_LENGTH 64
#define CFG_HOSTNAME_LENGTH 16

#define CFG_EMON_HOST_LENGTH 32
#define CFG_EMON_KEY_LENGTH 32
#define CFG_EMON_URL_LENGTH 32
#define CFG_EMON_DEFAULT_PORT 80
#define CFG_EMON_DEFAULT_HOST PSTR("emoncms.org")
#define CFG_EMON_DEFAULT_URL PSTR("/input/post.json")

#define CFG_JDOM_HOST_LENGTH 32
#define CFG_JDOM_KEY_LENGTH 48
#define CFG_JDOM_URL_LENGTH 64
#define CFG_JDOM_ADCO_LENGTH 12
#define CFG_JDOM_DEFAULT_PORT 80
#define CFG_JDOM_DEFAULT_HOST ""
#define CFG_JDOM_DEFAULT_URL PSTR("/plugins/teleinfo/core/php/jeeTeleinfo.php")
#define CFG_JDOM_DEFAULT_ADCO ""

#define CFG_HTTPREQ_HOST_LENGTH 32
#define CFG_HTTPREQ_URL_LENGTH 150
#define CFG_HTTPREQ_DEFAULT_PORT 80
#define CFG_HTTPREQ_DEFAULT_HOST ""
#define CFG_HTTPREQ_DEFAULT_URL PSTR("/json.htm?type=command&param=udevice&idx=1&nvalue=0&svalue=$HCHP;$HCHC;0;0;$PAPP;0")

// Port pour l'OTA
#define DEFAULT_OTA_PORT 8266
#define DEFAULT_OTA_AUTH PSTR("OTA_WifInfo")

#define CFG_LED_TINFO FPSTR("cfg_led_tinfo")
#define OPTION_LED_TINFO 0x0001 // blink led sur réception téléinfo

// Web Interface Configuration Form field names
#define CFG_FORM_SSID FPSTR("ssid")
#define CFG_FORM_PSK FPSTR("psk")
#define CFG_FORM_HOST FPSTR("host")
#define CFG_FORM_AP_PSK FPSTR("ap_psk")
#define CFG_FORM_OTA_AUTH FPSTR("ota_auth")
#define CFG_FORM_OTA_PORT FPSTR("ota_port")
#define CFG_FORM_SSE_FREQ FPSTR("sse_freq")

#define CFG_FORM_EMON_HOST FPSTR("emon_host")
#define CFG_FORM_EMON_PORT FPSTR("emon_port")
#define CFG_FORM_EMON_URL FPSTR("emon_url")
#define CFG_FORM_EMON_KEY FPSTR("emon_apikey")
#define CFG_FORM_EMON_NODE FPSTR("emon_node")
#define CFG_FORM_EMON_FREQ FPSTR("emon_freq")

#define CFG_FORM_JDOM_HOST FPSTR("jdom_host")
#define CFG_FORM_JDOM_PORT FPSTR("jdom_port")
#define CFG_FORM_JDOM_URL FPSTR("jdom_url")
#define CFG_FORM_JDOM_KEY FPSTR("jdom_apikey")
#define CFG_FORM_JDOM_ADCO FPSTR("jdom_adco")
#define CFG_FORM_JDOM_FREQ FPSTR("jdom_freq")

#define CFG_FORM_HTTPREQ_HOST FPSTR("httpreq_host")
#define CFG_FORM_HTTPREQ_PORT FPSTR("httpreq_port")
#define CFG_FORM_HTTPREQ_URL FPSTR("httpreq_url")
#define CFG_FORM_HTTPREQ_USE_POST FPSTR("httpreq_use_post")

#define CFG_FORM_HTTPREQ_FREQ FPSTR("httpreq_freq")
#define CFG_FORM_HTTPREQ_TRIGGER_PTEC FPSTR("httpreq_trigger_ptec")
#define CFG_FORM_HTTPREQ_TRIGGER_ADPS FPSTR("httpreq_trigger_adps")
#define CFG_FORM_HTTPREQ_TRIGGER_SEUILS FPSTR("httpreq_trigger_seuils")
#define CFG_FORM_HTTPREQ_SEUIL_HAUT FPSTR("httpreq_seuil_haut")
#define CFG_FORM_HTTPREQ_SEUIL_BAS FPSTR("httpreq_seuil_bas")

// Config for emoncms
// 128 Bytes
struct EmoncmsConfig
{
    char host[CFG_EMON_HOST_LENGTH + 1];  // FQDN
    char apikey[CFG_EMON_KEY_LENGTH + 1]; // Secret
    char url[CFG_EMON_URL_LENGTH + 1];    // URL
    uint16_t port;                        // port
    uint8_t node;                         // optional node
    uint32_t freq;                        // refresh rate
    uint8_t filler[22];
} __attribute__((packed));

// Config for jeedom
// 256 Bytes
struct JeedomConfig
{
    char host[CFG_JDOM_HOST_LENGTH + 1];  // FQDN
    char apikey[CFG_JDOM_KEY_LENGTH + 1]; // Secret
    char url[CFG_JDOM_URL_LENGTH + 1];    // URL
    char adco[CFG_JDOM_ADCO_LENGTH + 1];  // Identifiant compteur
    uint16_t port;                        // port
    uint32_t freq;                        // refresh rate
    uint8_t use_post;                     // POST un dictionnaire JSON
    uint8_t filler[89];
} __attribute__((packed));

// Config for http request
// 256 Bytes
struct HttpreqConfig
{
    char host[CFG_HTTPREQ_HOST_LENGTH + 1]; // FQDN
    char url[CFG_HTTPREQ_URL_LENGTH + 1];   // Path
    uint16_t port;                          // port
    uint32_t freq;                          // refresh rate
    uint8_t trigger_adps : 1;
    uint8_t trigger_ptec : 1;
    uint8_t trigger_seuils : 1;
    uint8_t unused : 4;   // pour remplir l'octet
    uint8_t use_post : 1; // POST au lieu de GET, la data est le dict JSON
    uint16_t seuil_haut;
    uint16_t seuil_bas;
    uint8_t filler[61];
} __attribute__((packed));

// Config saved into eeprom
// 1024 bytes total including CRC
struct Config
{
    char ssid[CFG_SSID_LENGTH + 1];     // SSID
    char psk[CFG_PSK_LENGTH + 1];       // Pre shared key
    char host[CFG_HOSTNAME_LENGTH + 1]; // Hostname
    char ap_psk[CFG_PSK_LENGTH + 1];    // Access Point Pre shared key
    char ota_auth[CFG_PSK_LENGTH + 1];  // OTA Authentication password
    uint32_t options;                   // Bit field register
    uint16_t ota_port;                  // OTA port
    uint16_t sse_freq;                  // fréquence mini des mises à jour SSE. 0=dès qu'une trame est dispo
    uint8_t filler[129];                // in case adding data in config avoiding loosing current conf by bad crc
    EmoncmsConfig emoncms;              // Emoncms configuration
    JeedomConfig jeedom;                // jeedom configuration
    HttpreqConfig httpreq;              // HTTP request
    uint16_t crc;
} __attribute__((packed));

// Exported variables/object instancied in main sketch
// ===================================================
extern Config config;

// Declared exported function from route.cpp
// ===================================================
bool config_read(bool clear_on_error = true);
bool config_save(void);
void config_show(void);
void config_get_json(String &r);
void config_handle_form(ESP8266WebServer &server);
void config_setup();
void config_reset();
