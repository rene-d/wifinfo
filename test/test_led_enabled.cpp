// module téléinformation client
// rene-d 2020

#include "mock.h"

#define ENABLE_LED
#include "led.h"

TEST(led_enabled, test)
{
    pinMode_called = 0;
    digitalRead_called = 0;
    digitalWrite_called = 0;

    led_setup();
    ASSERT_EQ(pinMode_called, 1); // uniquement un appel à pinMode
    ASSERT_EQ(digitalRead_called, 0);
    ASSERT_EQ(digitalWrite_called, 0);

    led_on();
    ASSERT_EQ(pinMode_called, 1);
    ASSERT_EQ(digitalRead_called, 0);
    ASSERT_EQ(digitalWrite_called, 1); // uniquement un appel à digitalWrite

    led_off();
    ASSERT_EQ(pinMode_called, 1);
    ASSERT_EQ(digitalRead_called, 0);
    ASSERT_EQ(digitalWrite_called, 2); // uniquement un appel à digitalWrite

    led_toggle();
    ASSERT_EQ(pinMode_called, 1);
    ASSERT_EQ(digitalRead_called, 1);  // un appel à digitalRead
    ASSERT_EQ(digitalWrite_called, 3); // et un appel à digitalWrite
}
