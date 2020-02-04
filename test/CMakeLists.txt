cmake_minimum_required(VERSION 3.16)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
find_package(nlohmann_json REQUIRED)
include(gtest.cmake)

project(tic)

set(CMAKE_CXX_STANDARD 14)
add_executable(tic
    test_tic.cpp test_config.cpp test_notifs.cpp test_support.cpp
    mock_time.cpp mock.cpp)
target_include_directories(tic
    PRIVATE ../src
    PRIVATE support)

target_link_libraries(tic
    PRIVATE gtest gtest_main
    PRIVATE nlohmann_json::nlohmann_json
)
