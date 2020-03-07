// module téléinformation client
// rene-d 2020

#include "wifinfo.h"
#include "timesync.h"
#include "cli.h"
#include "config.h"
#include "cpuload.h"
#include "filesystem.h"
#include "led.h"
#include "sys.h"
#include "teleinfo.h"
#include "tic.h"
#include "webserver.h"

#include <Arduino.h>
#include <ArduinoOTA.h>
#include <ESP8266mDNS.h>

void setup()
{
    led_setup();
    led_on();
    delay(100);

#ifdef ENABLE_DEBUG
    // en debug, on reste à 115200: on ne se branche pas au compteur
    Serial.begin(115200);
#else
    // sinon, RX est utilisé pour la téléinfo. TX peut toujours envoyer des diags
    Serial.begin(1200, SERIAL_7E1, SERIAL_RX_ONLY);
#endif
    Serial.flush();

    Serial.println(R"(
__      ___  __ ___       __
\ \    / (_)/ _|_ _|_ _  / _|___
 \ \/\/ /| |  _|| || ' \|  _/ _ \
  \_/\_/ |_|_| |___|_||_|_| \___/
)");
    Serial.flush();

    // chargement de la conf depuis l'EEPROM
    config_setup();

    // connexion au Wi-Fi ou activation de l'AP
    sys_wifi_connect();

    // initilisation du filesystem
    fs_setup();

#ifdef ENABLE_OTA
    // initialisation des mises à jour OTA
    sys_ota_setup();
#endif

    // démarrage client NTP
    time_setup();

    //MDNS.begin("esp8266");

#ifdef ENABLE_CLI
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
}

void loop()
{
#ifdef ENABLE_CPULOAD
    cpuload_loop();
    // return;
#endif

    webserver_loop();

#ifdef ENABLE_OTA
    ArduinoOTA.handle();
#endif

    // MDNS.update();

#ifdef ENABLE_CLI
    int c = cli_loop_read();
    if (c != -1)
    {
        // passe c à la téléinformation
        tic_decode(c);
    }
#else
    if (Serial.available())
    {
        int c = Serial.read();
        tic_decode(c);
    }
#endif
}
