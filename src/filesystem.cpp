#include "filesystem.h"

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
    bool first_item = true;

    response.reserve(512); // about 400 bytes

    // Files Array
    response = F("{\"files\":[");

    // Loop trough all files
    Dir dir = SPIFFS.openDir("/");
    while (dir.next())
    {
        String fileName = dir.fileName();
        size_t fileSize = dir.fileSize();

        if (first_item)
            first_item = false;
        else
            response += F(",");
        response += F("{\"na\":\"");
        response += fileName.c_str();
        response += F("\",\"va\":");
        response += fileSize;
        response += F("}");
    }
    response += F("],");

    // SPIFFS File system array
    response += F("\"spiffs\":[{");

    // Get SPIFFS File system informations
    FSInfo info;
    SPIFFS.info(info);
    response += F("\"Total\":");
    response += info.totalBytes;
    response += F(",\"Used\":");
    response += info.usedBytes;
    response += F(",\"ram\":");
    response += system_get_free_heap_size();
    response += F("}]");

    // Json end
    response += F("}");
}
