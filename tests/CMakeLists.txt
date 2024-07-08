include(CTest)

#
# Get required testing libraries
#
find_package(Catch2 REQUIRED)
# find_package(GTest CONFIG REQUIRED)

#
# Build neotests
#
FILE(GLOB_RECURSE TEST_SRCS CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp)
add_executable(neotests $<TARGET_OBJECTS:libneo> ${TEST_SRCS})
target_link_libraries(neotests PRIVATE
    CURL::libcurl
    Catch2::Catch2
    libneo
    pcapplusplus::pcapplusplus
    rapidjson
    tomlplusplus::tomlplusplus
)

# add_custom_target(neotests-setcap ALL
#    sudo setcap "cap_sys_admin+pe cap_net_admin+pe cap_net_raw+pe cap_sys_ptrace+pe" neotests
#    DEPENDS neotests)

add_test(NAME "neo_tests"
    COMMAND sudo "${CMAKE_CURRENT_BINARY_DIR}/neotests"
        --test-data-dir "${CMAKE_CURRENT_SOURCE_DIR}/networks"
        --durations yes)
