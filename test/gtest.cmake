include(ExternalProject)

set_directory_properties(PROPERTIES EP_PREFIX ${CMAKE_BINARY_DIR}/external)

ExternalProject_Add(
    googletest
    URL https://github.com/google/googletest/archive/release-1.10.0.zip
    INSTALL_COMMAND ""
    LOG_DOWNLOAD ON
    LOG_CONFIGURE ON
    LOG_BUILD ON)

ExternalProject_Get_Property(googletest source_dir)
ExternalProject_Get_Property(googletest binary_dir)

message(INFO "source_dir = ${source_dir}")

add_library(gtest UNKNOWN IMPORTED)
set_property(TARGET gtest PROPERTY IMPORTED_LOCATION ${binary_dir}/lib/libgtest.a)
set_property(TARGET gtest PROPERTY INTERFACE_INCLUDE_DIRECTORIES  ${source_dir}/googletest/include)
add_dependencies(gtest googletest)

add_library(gtest_main UNKNOWN IMPORTED)
set_property(TARGET gtest_main PROPERTY IMPORTED_LOCATION ${binary_dir}/lib/libgtest_main.a)
set_property(TARGET gtest_main PROPERTY INTERFACE_INCLUDE_DIRECTORIES ${source_dir}/googletest/include)
add_dependencies(gtest_main googletest)
