cmake_minimum_required(VERSION 3.13)

project(tic)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_EXTENSIONS OFF)

 find_library(GTEST NAMES gtest)

# if(GTEST-NOTFOUND)
#     find_package(gtest REQUIRED)
# else()
     set(GTEST_BOTH_LIBRARIES gtest gtest_main)
# endif()

# Code Coverage Configuration
add_library(coverage_config INTERFACE)

option(CODE_COVERAGE "Enable coverage reporting" OFF)
if(CODE_COVERAGE AND CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
  # Add required flags (GCC & LLVM/Clang)
  target_compile_options(coverage_config INTERFACE
    -O0        # no optimization
    -g         # generate debug info
    --coverage # sets all required flags
  )
  if(CMAKE_VERSION VERSION_GREATER_EQUAL 3.13)
    target_link_options(coverage_config INTERFACE --coverage)
  else()
    target_link_libraries(coverage_config INTERFACE --coverage)
  endif()
endif()

enable_testing()

add_executable(tic
    test/test_teleinfo.cpp
    test/test_config.cpp
    test/test_tic.cpp
    test/test_sys.cpp
    test/test_filesystem.cpp
    test/test_support.cpp
    test/mock_time.cpp
    test/mock.cpp
    test/mock_support.cpp)

target_include_directories(tic
    PRIVATE ${GTEST_INCLUDE_DIRS}
    PRIVATE src
    PRIVATE test/support)

target_link_libraries(tic
    PRIVATE ${GTEST_BOTH_LIBRARIES} pthread
    #PRIVATE nlohmann_json::nlohmann_json
)
target_compile_options(tic PUBLIC -Wall -pedantic -Wno-psabi)
target_link_libraries(tic PUBLIC coverage_config)

add_test(NAME tic_test COMMAND tic)
