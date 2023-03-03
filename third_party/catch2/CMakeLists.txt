# https://cmake.org/cmake/help/latest/policy/CMP0077.html
cmake_policy(VERSION 3.13)
set(CMAKE_POLICY_DEFAULT_CMP0077 NEW)

set(CATCH_INSTALL_DOCS OFF)
set(CATCH_DEVELOPMENT_BUILD OFF)

add_subdirectory(catch2)
