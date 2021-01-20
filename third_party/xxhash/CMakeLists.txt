add_library(xxhash STATIC xxhash.c)
target_include_directories(xxhash SYSTEM INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})

if (CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    target_compile_options(xxhash PUBLIC "-Wno-pass-failed")
endif()
