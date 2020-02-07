#pragma once

#include <Arduino.h>


enum LedColors
{
    COLOR_RED,
    COLOR_ORANGE,
    COLOR_MAGENTA,
};

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

inline void led_rgb_off()
{
    digitalWrite(LED_BUILTIN, HIGH);
}
