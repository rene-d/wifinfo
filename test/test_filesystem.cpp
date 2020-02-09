#include "mock.h"

#include "filesystem.cpp"

TEST(fs, init)
{
    SPIFFS.begin_called = false;
    fs_setup();

    ASSERT_TRUE(SPIFFS.begin_called);
}

TEST(fs, ls)
{
    fs_ls();
}

TEST(fs, json)
{
    String data;

    fs_get_spiffs_json(data);
    // std::cout << data << std::endl;

    auto j1 = json::parse(data.s);

    ASSERT_TRUE(j1["spiffs"].is_array());

    ASSERT_TRUE(j1["files"].is_array());
    ASSERT_EQ(j1["files"][0]["na"], "fichier1.txt");
    ASSERT_EQ(j1["files"][0]["va"], 1000);
}
