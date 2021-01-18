add_library(xxhash STATIC xxhash.c)
target_include_directories(xxhash SYSTEM INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})
