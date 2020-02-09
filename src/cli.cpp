#include "cli.h"
#include "timesync.h"
#include "sse.h"
#include "config.h"
#include "filesystem.h"
#include "tic.h"

#include <Arduino.h>
#include <SimpleCLI.h>
#include <PolledTimeout.h>
#include <EEPROM.h>

static void cli_eeprom_dump(uint8_t bytesPerRow, size_t size);

extern SseClients sse_clients;

static SimpleCLI cli;
static bool cli_mode = false;
static String cli_input;
static esp8266::polledTimeout::periodicMs timer_blink(esp8266::polledTimeout::periodicMs::neverExpires);

void cli_setup()
{
    cli.addSingleArgumentCommand("led", [](cmd *cmdPtr) {
        Command cmd(cmdPtr);
        const String &value = cmd.getArgument().getValue();
        if (value == "off")
        {
            timer_blink.resetToNeverExpires();
            digitalWrite(LED_BUILTIN, HIGH);
            Serial.println("led off");
        }
        else if (value == "on")
        {
            timer_blink.resetToNeverExpires();
            digitalWrite(LED_BUILTIN, LOW);
            Serial.println("led on");
        }
        else if (value == "blink")
        {
            timer_blink.reset(200);
        }
    });

    cli.addSingleArgumentCommand("sse", [](cmd *cmdPtr) {
        Command cmd(cmdPtr);
        String arg = cmd.getArgument().getValue();
        sse_clients.handle_clients(&arg);
    });

    cli.addSingleArgCmd("time", [](cmd *) {
        showTime();
    });

    cli.addSingleArgCmd("config", [](cmd *) {
        config_show();
    });

    cli.addSingleArgCmd("ls", [](cmd *) {
        fs_ls();
    });

    cli.addSingleArgCmd("dump", [](cmd *) {
        cli_eeprom_dump(16, 1024);
    });

    cli.addSingleArgCmd("set", [](cmd *cmdPtr) {
        Command cmd(cmdPtr);
        String arg = cmd.getArgument().getValue();

        if (arg == "timers")
        {
            tic_make_timers();
        }
    });

    cli.addSingleArgCmd("esp", [](cmd *cmdPtr) {
        Command cmd(cmdPtr);
        String arg = cmd.getArgument().getValue();

        if (arg == "restart")
        {
            Serial.println(F("restarting ESP..."));
            Serial.flush();

            ESP.restart();
            while (true)
            {
                delay(1);
            }
        }

        Serial.print("ChipId            : 0x");
        Serial.println(ESP.getChipId(), HEX);
        Serial.print("CpuFreqMHz        : ");
        Serial.println(ESP.getCpuFreqMHz());
        // Serial.print("Vcc               : ");
        // Serial.println(ESP.getVcc());

        Serial.print("SdkVersion        : ");
        Serial.println(ESP.getSdkVersion());
        Serial.print("CoreVersion       : ");
        Serial.println(ESP.getCoreVersion());
        Serial.print("FullVersion       : ");
        Serial.println(ESP.getFullVersion());

        Serial.print("FreeHeap          : ");
        Serial.println(ESP.getFreeHeap());
        Serial.print("MaxFreeBlockSize  : ");
        Serial.println(ESP.getMaxFreeBlockSize());
        Serial.print("HeapFragmentation : ");
        Serial.println(ESP.getHeapFragmentation());

        Serial.print(F("WiFi status     : "));
        Serial.println(WiFi.status());
        Serial.print(F("WiFi mode       : "));
        Serial.println(WiFi.getMode());
        Serial.print(F("localIP         : "));
        Serial.println(WiFi.localIP());
        Serial.print(F("hostname        : "));
        Serial.println(WiFi.hostname());
        Serial.print(F("softAPIP        : "));
        Serial.println(WiFi.softAPIP());
        Serial.flush();

        WiFi.printDiag(Serial);
        Serial.flush();
    });

    cli.setOnError([](cmd_error *errorPtr) {
        CommandError e(errorPtr);

        Serial.println("ERROR: " + e.toString());

        if (e.hasCommand())
        {
            Serial.println("Did you mean? " + e.getCommand().toString());
        }
        else
        {
            Serial.println(cli.toString());
        }
    });

    cli_input.reserve(64);
}

int cli_loop_read()
{
    if (timer_blink)
    {
        digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));
    }

    if (Serial.available())
    {
        int c = Serial.read();
        if ((c == '`') || (c == '~') || (c == 0x09))
        {
            cli_mode = true;
            Serial.print("$ ");
        }
        else if (cli_mode && (c >= 0) && (c <= 255))
        {
            Serial.write(c);
            cli_input += (char)c;
            if ((c == 0x0A) || (c == 0x0D) || (cli_input.length() >= 64))
            {
                cli.parse(cli_input);
                cli_input.clear();
                cli_mode = false;
                Serial.println();
            }
        }
        else
        {
            return c;
        }
    }
    return -1;
}

// dump eeprom value to serial
void cli_eeprom_dump(uint8_t bytesPerRow, size_t size)
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
