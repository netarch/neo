# https://cmake.org/cmake/help/latest/policy/CMP0077.html
cmake_policy(VERSION 3.13)
set(CMAKE_POLICY_DEFAULT_CMP0077 NEW)

set(RAPIDJSON_BUILD_DOC OFF)
set(RAPIDJSON_BUILD_EXAMPLES OFF)
set(RAPIDJSON_BUILD_TESTS OFF)
set(RAPIDJSON_BUILD_THIRDPARTY_GTEST OFF)
set(RAPIDJSON_BUILD_CXX11 ON)
set(RAPIDJSON_BUILD_CXX17 ON)
set(RAPIDJSON_BUILD_ASAN OFF)
set(RAPIDJSON_BUILD_UBSAN OFF)
set(RAPIDJSON_ENABLE_INSTRUMENTATION_OPT ON)

add_subdirectory(rapidjson)

find_path(RapidJSON_INCLUDE_DIRS
    NAMES rapidjson.h
    PATHS "${CMAKE_CURRENT_SOURCE_DIR}/rapidjson/include"
    PATH_SUFFIXES rapidjson
    NO_DEFAULT_PATH)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(RapidJSON
    FOUND_VAR RapidJSON_FOUND
    REQUIRED_VARS RapidJSON_INCLUDE_DIRS)

if (RapidJSON_FOUND AND NOT TARGET rapidjson)
    add_library(rapidjson INTERFACE IMPORTED)
    target_include_directories(rapidjson SYSTEM INTERFACE "${RapidJSON_INCLUDE_DIRS}")
endif()
