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
        Serial.println(F("FS failure"));
    }
    else
    {
        Serial.println(F("FS ok"));
    }
}

void fs_ls()
{
    int i = 0;
    Dir dir = WIFINFO_FS.openDir("/");
    while (dir.next())
    {
        const String &fileName = dir.fileName();
        size_t fileSize = dir.fileSize();
        Serial.printf("FS File: %2d %s, size: %zu\n", ++i, fileName.c_str(), fileSize);
    }
}

// Return JSON string containing list of files
void fs_get_json(String &response, bool restricted)
{
    if (restricted)
    {
        response = "{}";
        return;
    }

    response.reserve(512); // JSON is about 500 bytes

    response = F("{\"files\":");

    // Files
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
