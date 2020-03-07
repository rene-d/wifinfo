// module téléinformation client
// rene-d 2020

#include "wifinfo.h"
#include "filesystem.h"
#include "jsonbuilder.h"

#include <Arduino.h>
#include <FS.h>
#include <user_interface.h>

void fs_setup()
{
    // Init filesystem, to use web server static files
    if (!WIFINFO_FS.begin())
    {
        // Serious problem
        Serial.println(F("FS failure"));
    }
    else
    {
        Serial.println(F("FS ok"));
    }
}

void fs_ls()
{
    Dir dir = WIFINFO_FS.openDir("/");
    while (dir.next())
    {
        const String &fileName = dir.fileName();
        size_t fileSize = dir.fileSize();
        Serial.printf_P(PSTR("FS File: %s, size: %zu\n"), fileName.c_str(), fileSize);
    }
}

// Return JSON string containing list of SPIFFS files
void fs_get_json(String &response, bool restricted)
{
    if (restricted)
    {
        response = "{}";
        return;
    }

    response.reserve(512); // about 400 bytes

    response = F("{\"files\":");

    // SPIFFS File system array
    {
        JSONTableBuilder js(response);

        // Loop trough all files
        Dir dir = WIFINFO_FS.openDir("/");
        while (dir.next())
        {
            js.append(dir.fileName().c_str(), dir.fileSize());
        }

        js.finalize();
    }

    response += F(",\"info\":");

    // Filesystem information
    {
        JSONBuilder js(response);

        FSInfo info;
        WIFINFO_FS.info(info);

        js.append(F("Total"), info.totalBytes);
        js.append(F("Used"), info.usedBytes, true);
    }

    response += F("}");
}
