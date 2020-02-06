/*
 * Copyright (c) 2015-2020 rene-d. All right reserved.
 */

#include "timesync.h"
#include "sse.h"
#include "config.h"

#include <Arduino.h>
#include <SimpleCLI.h>
#include <PolledTimeout.h>

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
