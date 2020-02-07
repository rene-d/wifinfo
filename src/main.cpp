
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
#include <ArduinoOTA.h>
#include <ESP8266mDNS.h>

void setup()
{
    int led = digitalRead(LED_BUILTIN);
    pinMode(LED_BUILTIN, OUTPUT);
    digitalWrite(LED_BUILTIN, led);

#ifdef DEBUG
    // en debug, on reste à 115200: on ne se branche pas au compteur
    Serial.begin(115200);
#else
    // sinon, RX est utilisé pour la téléinfo. TX peut toujours envoyer des diags
    Serial.begin(1200, SERIAL_7E1);
#endif
    Serial.flush();

    led_on();
    delay(1000);

    // chargement de la conf depuis l'EEPROM
    config_setup();

    // connexion au Wi-Fi ou activation de l'AP
    sys_wifi_connect();

    // initilisation du filesystem
    fs_setup();

    // initialisation des mises à jour OTA
    sys_ota_setup();

    // démarrage client NTP
    time_setup();

    //MDNS.begin("esp8266");

#ifdef CLI_ENABLED
    // initialise le client série
    cli_setup();
#endif

    // initialise le serveur web embarqué
    webserver_setup();

    // active les timers de notification en fonction de la conf
    tic_make_timers();

    Serial.println();
    Serial.print(F("IP address: http://"));
    Serial.println(WiFi.localIP());
    Serial.flush();

    led_off();

    delay(100);
}

void loop()
{
    int c;

    webserver_loop();

    ArduinoOTA.handle();

    // MDNS.update();

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
