FILE(GLOB_RECURSE TEST_SRC_FILES CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp)

add_executable(neotests $<TARGET_OBJECTS:libneo> ${TEST_SRC_FILES})
target_link_libraries(neotests PRIVATE
    Catch2::Catch2
    libneo
    rapidjson
    tomlplusplus::tomlplusplus
)

add_custom_target(neotests-setcap ALL
    sudo setcap "cap_sys_admin+pe cap_net_admin+pe cap_net_raw+pe cap_sys_ptrace+pe" neotests
    DEPENDS neotests)

add_test(NAME "neo_tests"
    COMMAND sudo "${CMAKE_CURRENT_BINARY_DIR}/neotests"
        --test-data-dir "${CMAKE_CURRENT_SOURCE_DIR}/networks"
        --durations yes)
