#include "mock.h"

#include <Arduino.h>

TEST(support, String)
{
    String s1 = "test";
    ASSERT_EQ(s1.s, "test");

    String s2 = 123;
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
