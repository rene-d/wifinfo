#include "mock.h"

// WIP
#include "sys.cpp"

TEST(sys, get_json)
{
    String data;

    sys_get_info_json(data);

    // std::cout << data << std::endl;

    auto j1 = json::parse(data.s);

    ASSERT_TRUE(j1.is_array());
}
