#pragma once

#include <Arduino.h>

#define Debugln Serial.println
#define Debug Serial.print
#define Debugf Serial.printf
#define Debugf_P Serial.printf_P

#define DebugF Serial.print
#define DebuglnF Serial.println

enum LedColors
{
    COLOR_ORANGE,
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
