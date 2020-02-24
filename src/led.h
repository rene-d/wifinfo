// module téléinformation client
// rene-d 2020

#pragma once

#include <Arduino.h>

#ifdef DISABLE_LED

// pas de buildin led sur certains ESP-01

#define led_setup()
#define led_on()
#define led_off()

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

inline void led_toggle()
{
    digitalWrite(LED_BUILTIN, 1 - digitalRead(LED_BUILTIN));
}

#endif
