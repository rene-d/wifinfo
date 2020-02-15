// module téléinformation client
// rene-d 2020

#include "filesystem.h"
#include "jsonbuilder.h"

#include <Arduino.h>
#include <FS.h>
#include <user_interface.h>

void fs_setup()
{
    // Init SPIFFS filesystem, to use web server static files
    if (!SPIFFS.begin())
    {
        // Serious problem
        Serial.println(F("SPIFFS Mount failed !"));
    }
    else
    {
        Serial.println(F("SPIFFS Mount succesful"));
    }
}

void fs_ls()
{
    Dir dir = SPIFFS.openDir("/");
    while (dir.next())
    {
        const String &fileName = dir.fileName();
        size_t fileSize = dir.fileSize();
        Serial.printf("FS File: %s, size: %zu\n", fileName.c_str(), fileSize);
    }
}

// Return JSON string containing list of SPIFFS files
void fs_get_spiffs_json(String &response)
{
    response.reserve(512); // about 400 bytes

    response = F("{\"files\":");

    // SPIFFS File system array
    {
        JSONTableBuilder js(response);

        // Loop trough all files
        Dir dir = SPIFFS.openDir("/");
        while (dir.next())
        {
            js.append(dir.fileName().c_str(), dir.fileSize());
        }

        js.finalize();
    }

    response += F(",\"spiffs\":[");

    // SPIFFS File system informations
    {
        JSONBuilder js(response);

        FSInfo info;
        SPIFFS.info(info);

        js.append(F("Total"), info.totalBytes);
        js.append(F("Used"), info.usedBytes);
        js.append(F("RAM"), system_get_free_heap_size(), true);
    }

    response += F("]}");
}
