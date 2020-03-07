// module téléinformation client
// rene-d 2020

#include "mock.h"

#include "filesystem.cpp"

TEST(fs, init)
{
    ERFS.begin_called = false;
    fs_setup();

    ASSERT_TRUE(ERFS.begin_called);
}

TEST(fs, ls)
{
    fs_ls();
}

TEST(fs, json)
{
    String data;

    fs_get_json(data, false);
    // std::cout << data << std::endl;

    auto j1 = json::parse(data.s);

    ASSERT_TRUE(j1["info"].is_object());

    ASSERT_TRUE(j1["files"].is_array());
    ASSERT_EQ(j1["files"][0]["na"], "fichier1.txt");
    ASSERT_EQ(j1["files"][0]["va"], 1000);
}

TEST(fs, json_restricted)
{
    String data;

    fs_get_json(data, true);

    ASSERT_EQ(data.s, "{}");
}
