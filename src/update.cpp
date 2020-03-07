// module téléinformation client
// rene-d 2020

#include "wifinfo.h"
#include "led.h"
#include "sys.h"
#include "webserver.h"
#include <ESP8266WebServer.h>
#include <Ticker.h>

static bool sys_update_is_ok = false; // indicates a successful update
static Ticker blink;

static const char update_html[] PROGMEM = R"html(<!DOCTYPE html>
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
</html>)html";

extern "C" uint32_t _FS_start;
extern "C" uint32_t _FS_end;

//
static void sys_update_finish(ESP8266WebServer &server, bool finish = false)
{
    Serial.println(F("sys_update_finish"));
    led_off();
    blink.detach();

    if (Update.hasError())
    {
        Serial.print("Update error: ");
#ifdef ENABLE_DEBUG
        Update.printError(Serial);
#endif
    }

    sys_update_is_ok = Update.end(finish);

    Serial.printf("sys_update_is_ok %d\n", sys_update_is_ok);

    server.sendHeader("Connection", "close");
    server.sendHeader("Access-Control-Allow-Origin", "*");
    server.send(200, mime::mimeTable[mime::txt].mimeType, sys_update_is_ok ? "OK" : "FAIL");
}

//
void sys_update_register(ESP8266WebServer &server)
{
    // page statique si on a perdu le filesystem...
    server.on(
        "/update",
        HTTP_GET,
        [&]() {
            if (webserver_access_full())
            {
                server.send(200, mime::mimeTable[mime::html].mimeType, update_html);
            }
        });

    // handler for the /update form POST (once file upload finishes)
    server.on(
        "/update",
        HTTP_POST,

        // handler once file upload finishes
        [&]() {
            if (webserver_access_full())
            {
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
                    Serial.println(F("upload terminated: update has error"));
                }
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
                    led_toggle();
                });

                sys_update_is_ok = false;
                int command = (upload.filename.indexOf("spiffs.bin") != -1) ? U_FS : U_FLASH;

                // upload.contentLength is NOT the real upload size
                uint32_t max_size;
                if (command == U_FS)
                {
                    // contentLength is a little above the authorized length
                    max_size = (uint32_t)&_FS_end - (uint32_t)&_FS_start;
                }
                else
                {
                    // should be always ok
                    max_size = upload.contentLength;

#ifdef ENABLE_DEBUG
                    uintptr_t updateStartAddress = 0;
                    size_t currentSketchSize = (ESP.getSketchSize() + FLASH_SECTOR_SIZE - 1) & (~(FLASH_SECTOR_SIZE - 1));
                    size_t roundedSize = (max_size + FLASH_SECTOR_SIZE - 1) & (~(FLASH_SECTOR_SIZE - 1));

                    //address of the end of the space available for sketch and update
                    uintptr_t updateEndAddress = (uintptr_t)&_FS_start - 0x40200000;
                    updateStartAddress = (updateEndAddress > roundedSize) ? (updateEndAddress - roundedSize) : 0;

                    Serial.printf_P(PSTR("[begin] max_size:           0x%08zX (%zd)\n"), max_size, max_size);
                    Serial.printf_P(PSTR("[begin] roundedSize:        0x%08zX (%zd)\n"), roundedSize, roundedSize);
                    Serial.printf_P(PSTR("[begin] updateStartAddress: 0x%08zX (%zd)\n"), updateStartAddress, updateStartAddress);
                    Serial.printf_P(PSTR("[begin] updateEndAddress:   0x%08zX (%zd)\n"), updateEndAddress, updateEndAddress);
                    Serial.printf_P(PSTR("[begin] currentSketchSize:  0x%08zX (%zd)\n"), currentSketchSize, currentSketchSize);
#endif
                }

                if (!Update.begin(max_size, command))
                {
                    Serial.printf("begin error %d %d\n", max_size, command);
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
