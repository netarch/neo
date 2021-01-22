FILE(GLOB_RECURSE TEST_SRC_FILES CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp)

add_executable(unittest $<TARGET_OBJECTS:libneo> ${TEST_SRC_FILES})
target_link_libraries(unittest PRIVATE libneo Catch2::Catch2)
target_link_libraries(unittest PRIVATE spin_model cpptoml xxhash Libnet::Libnet
    Threads::Threads)

add_custom_target(unittest-setcap ALL
    sudo setcap "cap_sys_admin+pe cap_net_admin+pe cap_net_raw+pe cap_sys_ptrace+pe" unittest
    DEPENDS unittest)

add_test(NAME "neo_unit_test"
    COMMAND "${CMAKE_CURRENT_BINARY_DIR}/unittest"
    WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}")
