#include "webserver.h"
#include "filesystem.h"
#include "sys.h"
#include "config.h"
#include "sse.h"
#include "tic.h"

#include <Arduino.h>
#include <ESP8266WebServer.h>

ESP8266WebServer server(80);
SseClients sse_clients;

template <void (*get_json)(String &)>
void server_send_json()
{
    String response;

    Serial.printf("server %s page\n", server.uri().c_str());

    ESP.wdtFeed(); //Force software watchdog to restart from 0
    get_json(response);
    if (response.isEmpty())
    {
        server.send(200, "text/json", "{}");
        // server.send(404, "text/plain", "no data");
    }
    else
    {
        server.send(200, "text/json", response);
    }
    yield(); //Let a chance to other threads to work
}

void webserver_handle_notfound()
{
    const char *value = tic_get_value(server.uri().c_str() + 1);

    if (value != nullptr)
    {
        String response = F("{\"");
        response += server.uri().c_str() + 1;
        response += F("\":\"");
        response += value;
        response += F("\"}");
        server.send(200, "text/json", response);
    }
    else
    {
        // send error message in plain text
        String message = F("File Not Found\n\n");
        message += F("URI: ");
        message += server.uri();
        message += F("\nMethod: ");
        message += (server.method() == HTTP_GET) ? "GET" : "POST";
        message += F("\nArguments: ");
        message += server.args();
        message += F("\n");

        for (uint8_t i = 0; i < server.args(); i++)
        {
            message += " " + server.argName(i) + ": " + server.arg(i) + F("\n");
        }

        server.send(404, "text/plain", message);
    }
}

void webserver_setup()
{
    //Server Sent Events will be handled from this URI
    sse_clients.on("/sse/tinfo.json", server);

    server.on("/config_form.json", [] { config_handle_form(server); });
    server.on("/json", server_send_json<tic_get_json_dict>);
    server.on("/tinfo.json", server_send_json<tic_get_json_array>);
    server.on("/emoncms.json", server_send_json<tic_emoncms_data>);
    server.on("/system.json", server_send_json<sys_get_info_json>);
    server.on("/config.json", server_send_json<config_get_json>);
    server.on("/spiffs.json", server_send_json<fs_get_spiffs_json>);
    server.on("/wifiscan.json", server_send_json<sys_wifi_scan_json>);
    server.on("/factory_reset", [] { sys_handle_factory_reset(server); });
    server.on("/reset", [] { sys_handle_reset(server); });

    // handler for the hearbeat
    server.on("/hb.htm", HTTP_GET, [] {
        server.sendHeader("Connection", "close");
        server.sendHeader("Access-Control-Allow-Origin", "*");
        server.send(200, "text/html", "OK");
    });

    // serves all SPIFFS Web file with 24hr max-age control
    // to avoid multiple requests to ESP
    server.serveStatic("/font", SPIFFS, "/font", "max-age=86400");
    server.serveStatic("/js", SPIFFS, "/js", "max-age=86400");
    server.serveStatic("/css", SPIFFS, "/css", "max-age=86400");
    server.serveStatic("/version", SPIFFS, "/version", "max-age=86400");

#ifdef ENABLE_OTA
    // enregistre le handler de /update
    sys_ota_register(server);
#endif

    server.onNotFound(webserver_handle_notfound);

    server.serveStatic("/", SPIFFS, "/", "max-age=86400");

    //start the webserver
    server.begin();
}

void webserver_loop()
{
    server.handleClient();
    sse_clients.handle_clients();
}
