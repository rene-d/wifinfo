// module téléinformation client
// rene-d 2020

#pragma once

#ifndef PLATFORMIO

// version logicielle
// PlatformIO positionne automatiquement à partir de la référence Git
#define WIFINFO_VERSION "arduinoide"

// active la sortie sur le port série TX et vitesse 115200.
// Non utilisable avec un compteur, il faut utiliser le client de test pour injecter des trames.
// #define ENABLE_DEBUG

// active l'utilisation de LED pour les cartes qui en ont une (esp01s, esp12e)
#define ENABLE_LED

// active les commandes par port série (`TAB` ou `ESC`)
// #define ENABLE_CLI

// rajoute le code pour les mises à jour OTA **(non testé)**
// #define ENABLE_OTA

// mesure de manière empirique la charge CPU
// #define ENABLE_CPULOAD

#endif

//
#define WIFINFO_FS SPIFFS
//#include "EPFS.h"
