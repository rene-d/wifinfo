// module téléinformation client
// rene-d 2020

#pragma once

#include <Arduino.h>

enum LedColors
{
    COLOR_RED,
    COLOR_ORANGE,
    COLOR_MAGENTA,
};

#ifdef DISABLE_LED

// pas de buildin led sur certains ESP-01

#define led_setup()
#define led_on()
#define led_off()
#define led_rgb_on(x)
#define led_rgb_toggle(x)
#define led_rgb_off()

#else

inline void led_setup()
{
    pinMode(LED_BUILTIN, OUTPUT);
}

inline void led_on()
{
    digitalWrite(LED_BUILTIN, LOW);
}

inline void led_off()
{
    digitalWrite(LED_BUILTIN, HIGH);
}

inline void led_rgb_on(LedColors color)
{
    digitalWrite(LED_BUILTIN, LOW);
}

inline void led_rgb_toggle(LedColors color)
{
    digitalWrite(LED_BUILTIN, 1 - digitalRead(LED_BUILTIN));
}

inline void led_rgb_off()
{
    digitalWrite(LED_BUILTIN, HIGH);
}

#endif
