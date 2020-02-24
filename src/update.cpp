#include "led.h"
#include "sys.h"
#include <ESP8266WebServer.h>
#include <Ticker.h>

static bool sys_update_is_ok = false; // indicates a successful update
static Ticker blink;

//
static void sys_update_finish(ESP8266WebServer &server, bool finish = false)
{
    Serial.println(F("sys_update_finish"));
    led_rgb_off();
    blink.detach();

    if (Update.hasError())
    {
        Update.printError(Serial);
    }

    sys_update_is_ok = Update.end(finish);

    Serial.printf("sys_update_is_ok %d\n", sys_update_is_ok);

    server.sendHeader("Connection", "close");
    server.sendHeader("Access-Control-Allow-Origin", "*");
    server.send(200, "text/plain", sys_update_is_ok ? "OK" : "FAIL");
}

//
void sys_update_register(ESP8266WebServer &server)
{
    // page statique si on a perdu le filesystem...
    server.on(
        "/update",
        HTTP_GET,
        [&]() {
            server.send(200, "text/html", R"html(<!DOCTYPE html>
<html>
<head>
<title>Upload firmware or filesystem</title>
<style>body { font-family: sans-serif; color: #444; background-color: #ddd; }</style>
</head>
<body>
<p>Select a new file to upload to the ESP8266</p>
<form method="POST" enctype="multipart/form-data">
<input type="file" name="data">
<input class="button" type="submit" value="Upload">
</form>
</body>
</html>)html");
        });

    // handler for the /update form POST (once file upload finishes)
    server.on(
        "/update",
        HTTP_POST,

        // handler once file upload finishes
        [&]() {
            if (sys_update_is_ok)
            {
                Serial.println(F("upload terminated: restart"));
                Serial.flush();

                // reboot dans 0.5s
                blink.once_ms(500, [] {
                    ESP.restart();
                });
            }
            else
            {
                Serial.println(F("upload terminated: Update has error"));
            }
        },

        // handler for upload, get's the sketch bytes,
        // and writes them through the Update object
        [&]() {
            HTTPUpload &upload = server.upload();

            if (upload.status == UPLOAD_FILE_START)
            {
                Serial.printf_P(PSTR("upload start: %s (%u bytes)\n"), upload.filename.c_str(), upload.contentLength);
                blink.attach_ms(333, [] {
                    led_rgb_toggle(COLOR_MAGENTA);
                });

                sys_update_is_ok = false;
                int command = (upload.filename.indexOf("spiffs.bin") != -1) ? U_FS : U_FLASH;

                if (!Update.begin(1024000, command))
                {
                    Serial.println(F("begin error"));
                    sys_update_finish(server);
                }
            }
            else if (upload.status == UPLOAD_FILE_WRITE)
            {
                Serial.print('.');

                if (Update.write(upload.buf, upload.currentSize) != upload.currentSize)
                {
                    Serial.println(F("write error"));

                    sys_update_finish(server);
                }
            }
            else if (upload.status == UPLOAD_FILE_END)
            {
                Serial.println(F("upload end"));
                sys_update_finish(server, true);
            }
            else if (upload.status == UPLOAD_FILE_ABORTED)
            {
                Serial.println(F("upload aborted"));
                sys_update_finish(server);
            }
        });
}
