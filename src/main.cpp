
#include "timesync.h"
#include "cli.h"
#include "filesystem.h"
#include "config.h"
#include "led.h"
#include "sys.h"
#include "teleinfo.h"
#include "webserver.h"
#include "tic.h"

#include <Arduino.h>
#include <ESP8266mDNS.h>

void setup()
{
    int led = digitalRead(LED_BUILTIN);
    pinMode(LED_BUILTIN, OUTPUT);
    digitalWrite(LED_BUILTIN, led);

    // Set CPU speed to 160MHz
    system_update_cpu_freq(160);

    // Open serial communications and wait for port to open:
    Serial.begin(115200);
    Serial.flush();

    led_on();
    delay(1000);

    fs_setup();
    config_setup();

    // start Wifi connect or soft AP
    sys_wifi_connect(true);

    time_setup();

    MDNS.begin("esp8266");

    cli_setup();

    webserver_setup();

    tic_make_timers();

    delay(100);

    Serial.println();
    Serial.print(F("IP address: "));
    Serial.println(WiFi.localIP());
    Serial.flush();

    led_off();
}

void loop()
{
    int c;

    webserver_loop();

    MDNS.update();

#ifdef CLI_ENABLED
    c = cli_loop_read();
    if (c != -1)
    {
        // passe c à la téléinformation
        tic_decode(c);
    }
#else
    if (Serial.available())
    {
        c = Serial.read();
        tic_decode(c);
    }
#endif
}
