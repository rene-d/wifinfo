#include "cli.h"
#include "timesync.h"
#include "sse.h"
#include "config.h"
#include "filesystem.h"
#include "tic.h"
#include "sys.h"

#include <Arduino.h>
#include <SimpleCLI.h>
#include <PolledTimeout.h>
#include <EEPROM.h>

extern "C" uint32_t _EEPROM_start;

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
        time_show();
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

    cli.addSingleArgCmd("tic", [](cmd *) {
        tic_dump();
    });

    cli.addBoundlessCmd(("set"), [](cmd *cmdPtr) {
        Command cmd(cmdPtr);

        if (cmd.countArgs() == 0)
        {
            Serial.println(F("nothing to set"));
            return;
        }

        String arg = cmd.getArgument(0).getValue();

        if (arg == F("timers"))
        {
            tic_make_timers();
        }

        else if ((arg == F("wifi")) && (cmd.countArgs() >= 1))
        {
            strncpy(config.ssid, cmd.getArg(1).getValue().c_str(), CFG_SSID_LENGTH);
            strncpy(config.psk, cmd.getArg(2).getValue().c_str(), CFG_PSK_LENGTH);
            config_save();

            ESP.restart();
        }
        else if ((arg == F("ap")) && (cmd.countArgs() >= 1))
        {
            strncpy(config.host, cmd.getArg(1).getValue().c_str(), CFG_HOSTNAME_LENGTH);
            strncpy(config.ap_psk, cmd.getArg(2).getValue().c_str(), CFG_PSK_LENGTH);
            config_save();

            ESP.restart();
        }

        else if (arg == F("gpio") && cmd.countArgs() >= 2)
        {
            uint8_t pin = cmd.getArg(1).getValue().toInt();

            if (cmd.countArgs() == 2)
            {
                uint8_t v = digitalRead(pin);
                Serial.printf_P(PSTR("gpio %d read %d\n"), pin, v);
            }
            else
            {
                uint8_t v = cmd.getArg(2).getValue().toInt();
                Serial.printf_P(PSTR("gpio %d write %d\n"), pin, v);
                digitalWrite(pin, v);
            }
        }
    });

    cli.addSingleArgCmd("esp", [](cmd *cmdPtr) {
        Command cmd(cmdPtr);
        String arg = cmd.getArgument().getValue();

        if (arg == "restart")
        {
            Serial.println(F("restart..."));
            Serial.flush();

            ESP.restart();
            while (true)
            {
                delay(1);
            }
        }

        if (arg == "reset")
        {
            Serial.println(F("eraseConfig..."));
            ESP.eraseConfig();
            Serial.println(F("WiFi.disconnect..."));
            WiFi.disconnect(true);

            Serial.println(F("clear EEPROM..."));
            for (int i = 0; i < 1024; ++i)
                EEPROM.write(i, 0);
            EEPROM.commit();

            delay(500);

            Serial.println(F("restart..."));
            Serial.flush();

            ESP.restart();
            while (true)
            {
                delay(1);
            }
        }

        Serial.printf_P(PSTR("ChipId      : 0x%06X\n"), ESP.getChipId());
        Serial.printf_P(PSTR("CpuFreqMHz  : %d MHz\n"), ESP.getCpuFreqMHz());
        Serial.printf_P(PSTR("Vcc         : %u\n"), ESP.getVcc());
        Serial.printf_P(PSTR("ResetReason : %s\n"), ESP.getResetReason().c_str());
        Serial.printf_P(PSTR("ResetInfo   : %s\n"), ESP.getResetInfo().c_str());

        Serial.printf_P(PSTR("SdkVersion  : %s\n"), ESP.getSdkVersion());
        Serial.printf_P(PSTR("CoreVersion : %s\n"), ESP.getCoreVersion().c_str());
        Serial.printf_P(PSTR("FullVersion : %s\n"), ESP.getFullVersion().c_str());

        Serial.printf_P(PSTR("SketchSize        : %u\n"), ESP.getSketchSize());
        Serial.printf_P(PSTR("FreeSketchSpace   : %u\n"), ESP.getFreeSketchSpace());

        Serial.printf_P(PSTR("FlashChipRealSize : %u\n"), ESP.getFlashChipRealSize());
        Serial.printf_P(PSTR("checkFlashConfig  : %d\n"), ESP.checkFlashConfig());
        Serial.printf_P(PSTR("FlashChipVendorId : 0x%x\n"), ESP.getFlashChipVendorId());
        Serial.printf_P(PSTR("EEPROM_start      : 0x%08x\n"), (uint32_t)&_EEPROM_start - 0x40200000);

        Serial.printf_P(PSTR("FreeHeap          : %u\n"), ESP.getFreeHeap());
        Serial.printf_P(PSTR("MaxFreeBlockSize  : %u\n"), ESP.getMaxFreeBlockSize());
        Serial.printf_P(PSTR("HeapFragmentation : %u\n"), ESP.getHeapFragmentation());

        FSInfo info;
        SPIFFS.info(info);
        Serial.printf_P(PSTR("SPIFFS totalBytes    : %zu\n"), info.totalBytes);
        Serial.printf_P(PSTR("SPIFFS usedBytes     : %zu\n"), info.usedBytes);
        Serial.printf_P(PSTR("SPIFFS blockSize     : %zu\n"), info.blockSize);
        Serial.printf_P(PSTR("SPIFFS pageSize      : %zu\n"), info.pageSize);
        Serial.printf_P(PSTR("SPIFFS maxOpenFiles  : %zu\n"), info.maxOpenFiles);
        Serial.printf_P(PSTR("SPIFFS maxPathLength : %zu\n"), info.maxPathLength);

        Serial.printf_P(PSTR("WiFi status : %d\n"), WiFi.status());
        Serial.printf_P(PSTR("WiFi mode   : %d\n"), WiFi.getMode());
        Serial.printf_P(PSTR("localIP     : %s\n"), WiFi.localIP().toString().c_str());
        Serial.printf_P(PSTR("hostname    : %s\n"), WiFi.hostname().c_str());
        Serial.printf_P(PSTR("softAPIP    : %s\n"), WiFi.softAPIP().toString().c_str());
        Serial.printf_P(PSTR("Persistent  : %d\n"), WiFi.getPersistent());

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
        if ((c == 0x09) || (c == 0x1B)) // ESC non présent dans une trame de téléinfo
        {                               // TAB possible en séparateur, mais pas rencontré
            cli_mode = true;
            Serial.print("$ ");
        }
        else if (cli_mode && (c >= 0) && (c <= 255))
        {
            Serial.write(c);
            cli_input += (char)c;
            if ((c == 0x0A) || (c == 0x0D) || (cli_input.length() >= 128))
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
