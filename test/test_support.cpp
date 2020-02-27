// module téléinformation client
// rene-d 2020

#include "mock.h"

#include <Arduino.h>

TEST(support, String)
{
    String s1 = "test";
    ASSERT_EQ(s1.s, "test");

    String s2;
    s2 = 123;
    ASSERT_EQ(s2.s, "123");

    String s3 = "test";
    s3 += 123;
    ASSERT_EQ(s3.s, "test123");

    String s4 = "test";
    s4 += "test";
    ASSERT_EQ(s4.s, "testtest");

    String s5 = "test";
    String s6 = "xyz";
    s5 += s6;
    ASSERT_EQ(s5.s, "testxyz");
}

TEST(support, SerialClass)
{
    String lol = "lol";

    SerialClass::buffer.clear();
    SerialClass::flush_called = 0;

    Serial.flush();
    EXPECT_EQ(SerialClass::flush_called, 1);
    EXPECT_EQ(SerialClass::buffer.length(), 0u);
    SerialClass::flush_called = 0;
    SerialClass::buffer.clear();

    Serial.println();
    EXPECT_EQ(SerialClass::buffer, "\n");
    SerialClass::buffer.clear();

    Serial.print("toto");
    EXPECT_EQ(SerialClass::buffer, "toto");
    SerialClass::buffer.clear();

    Serial.println("toto");
    EXPECT_EQ(SerialClass::buffer, "toto\n");
    SerialClass::buffer.clear();

    Serial.print(lol);
    EXPECT_EQ(SerialClass::buffer, "lol");
    SerialClass::buffer.clear();

    Serial.println(lol);
    EXPECT_EQ(SerialClass::buffer, "lol\n");
    SerialClass::buffer.clear();

    Serial.print('a');
    EXPECT_EQ(SerialClass::buffer, "a");
    SerialClass::buffer.clear();

    Serial.println('a');
    EXPECT_EQ(SerialClass::buffer, "a\n");
    SerialClass::buffer.clear();

    Serial.print("a");
    Serial.println("b");
    Serial.println(10);
    EXPECT_EQ(SerialClass::buffer, "ab\n10\n");

    EXPECT_EQ(SerialClass::flush_called, 0);
}
