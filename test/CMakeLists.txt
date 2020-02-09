cmake_minimum_required(VERSION 3.13)

project(tic)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_EXTENSIONS OFF)


# find_package(gtest REQUIRED)
# include_directories(/usr/local/include)
# link_directories(/usr/local/lib)

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
    test_teleinfo.cpp
    test_config.cpp
    test_tic.cpp
    test_sys.cpp
    test_support.cpp mock_time.cpp mock.cpp mock_support.cpp)

target_include_directories(tic
    PRIVATE src
    PRIVATE support)

target_link_libraries(tic
    PRIVATE gtest gtest_main
    #PRIVATE nlohmann_json::nlohmann_json
)

target_compile_options(tic PUBLIC -Wall -pedantic)

# Include code-coverage settings:
target_link_libraries(tic PUBLIC coverage_config)

add_test(NAME tic_test COMMAND tic)
