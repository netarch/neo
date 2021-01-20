FILE(GLOB_RECURSE TEST_SRC_FILES CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp)
set(SRC_FILES_WO_MAIN ${SRC_FILES})
list(REMOVE_ITEM SRC_FILES_WO_MAIN ${SRC_DIR}/main.cpp)

add_executable(unittest ${SRC_FILES_WO_MAIN} ${TEST_SRC_FILES})
add_dependencies(unittest spdlog)
target_include_directories(unittest PRIVATE ${SRC_DIR})
target_include_directories(unittest SYSTEM PRIVATE ${spdlog_INCLUDE_DIRS})
target_link_libraries(unittest PRIVATE spin_model cpptoml xxhash Libnet::Libnet
  Threads::Threads Catch2::Catch2)

add_custom_target(unittest-setcap ALL
    sudo setcap "cap_sys_admin+pe cap_net_admin+pe cap_net_raw+pe cap_sys_ptrace+pe" neo
    DEPENDS unittest)

add_test(NAME "neo unit test"
    COMMAND "${CMAKE_CURRENT_BINARY_DIR}/unittest"
    WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}")
