# module téléinformation client
# rene-d 2020

cmake_minimum_required(VERSION 3.10)

project(tic)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_EXTENSIONS OFF)

option(DUMP_VARS "dump CMake variables" OFF)


if(CMAKE_HOST_SYSTEM_NAME STREQUAL Darwin)
    find_package(gtest REQUIRED)
else()
    set(GTEST_BOTH_LIBRARIES gtest gtest_main)

    # debian/ubuntu
    if(EXISTS /usr/src/googletest/CMakeLists.txt)
        add_subdirectory(/usr/src/googletest external EXCLUDE_FROM_ALL)
    endif()
endif()


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

#
#
add_executable(tic
    test/test_support.cpp
    test/test_teleinfo.cpp
    test/test_config.cpp
    test/test_tic.cpp
    test/test_sys.cpp
    test/test_filesystem.cpp
    test/mock_time.cpp
    test/mock.cpp
    test/mock_support.cpp)

target_include_directories(tic
    PRIVATE ${GTEST_INCLUDE_DIRS}
    PRIVATE src
    PRIVATE test/support)

target_compile_options(tic PUBLIC -Wall -pedantic)

if(CMAKE_SYSTEM_PROCESSOR STREQUAL armv7l)
    # https://github.com/nlohmann/json/issues/658
    target_compile_options(tic PUBLIC -Wno-psabi)
endif()

target_link_libraries(tic
    PRIVATE ${GTEST_BOTH_LIBRARIES} pthread)

target_link_libraries(tic PUBLIC coverage_config)

target_compile_definitions(tic PRIVATE ENABLE_CLI DEBUG)

#
#
add_executable(gen_eeprom
    test/gen_eeprom.cpp test/mock_support.cpp)
target_include_directories(gen_eeprom
    PRIVATE src
    PRIVATE test/support)


#
#
enable_testing()
add_test(NAME tic_test COMMAND tic --gtest_output=xml)

#
#
function(dump_cmake_variables)
    get_cmake_property(_variableNames VARIABLES)
    list(SORT _variableNames)
    foreach (_variableName ${_variableNames})
        message(STATUS "${_variableName}=${${_variableName}}")
    endforeach()
endfunction()
if(DUMP_VARS)
    dump_cmake_variables()
endif()
