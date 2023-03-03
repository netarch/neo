cmake_minimum_required(VERSION 3.13)
project(Neo VERSION 0.1 HOMEPAGE_URL https://github.com/netarch/neo LANGUAGES C CXX)
include("CheckTypeSize")
include("ExternalProject")

#
# compile options
#
set(CMAKE_C_STANDARD 17)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(VECTORSZ 2048 CACHE STRING "Maximum number of communications")
set(MAX_CONNS 10 CACHE STRING "Maximum number of communications")
check_type_size("void *" SIZEOF_VOID_P)
check_type_size("int" SIZEOF_INT)
add_compile_options(-Wall -Wextra -Werror -O2)

if (CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    add_compile_options(-Wno-c99-designator)
endif()

# See http://spinroot.com/spin/Man/Pan.html
add_compile_definitions(
    NIBIS
    NOBOUNDCHECK
    NOCOMP
    NOFAIR
    SAFETY
    SFH
    VECTORSZ=${VECTORSZ}
    MAX_CONNS=${MAX_CONNS}
    SIZEOF_VOID_P=${SIZEOF_VOID_P}
    SIZEOF_INT=${SIZEOF_INT})
set(SPIN_FLAGS -a -DMAX_CONNS=${MAX_CONNS} -DSIZEOF_VOID_P=${SIZEOF_VOID_P}
    -DSIZEOF_INT=${SIZEOF_INT})

#
# release/debug compile options
#
set(CMAKE_C_FLAGS_RELEASE "-O3")
set(CMAKE_CXX_FLAGS_RELEASE "-O3")
set(CMAKE_C_FLAGS_RELWITHDEBINFO "-g -O3")
set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "-g -O3")
set(CMAKE_C_FLAGS_DEBUG "-DENABLE_DEBUG -g -fsanitize=address -fno-omit-frame-pointer -fno-common")
set(CMAKE_CXX_FLAGS_DEBUG "-DENABLE_DEBUG -g -fsanitize=address -fno-omit-frame-pointer -fno-common")
set(CMAKE_EXE_LINKER_FLAGS_DEBUG "-fsanitize=address")
if ("${CMAKE_BUILD_TYPE}" STREQUAL "")
    set(CMAKE_BUILD_TYPE RelWithDebInfo)
endif()

#
# coverage options
#
add_library(coverage_config INTERFACE)
option(ENABLE_COVERAGE "Enable coverage testing" OFF)
if (ENABLE_COVERAGE)
    target_compile_options(coverage_config INTERFACE -O0 -g --coverage)
    if(CMAKE_VERSION VERSION_GREATER_EQUAL 3.13)
        target_link_options(coverage_config INTERFACE --coverage)
    else()
        target_link_libraries(coverage_config INTERFACE --coverage)
    endif()
endif()

#
# third party libraries
#
list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")
add_subdirectory(third_party/catch2)
add_subdirectory(third_party/rapidjson)
add_subdirectory(third_party/spdlog)
add_subdirectory(third_party/tomlplusplus)
set(XXHASH_BUILD_ENABLE_INLINE_API ON)
set(XXHASH_BUILD_XXHSUM OFF)
set(BUILD_SHARED_LIBS OFF)
add_subdirectory(third_party/xxhash/cmake_unofficial EXCLUDE_FROM_ALL)
find_package(CURL REQUIRED)
find_package(Libnet REQUIRED)
find_package(Libnl REQUIRED)
find_package(PcapPlusPlus REQUIRED)
find_package(RapidJSON REQUIRED)
find_package(Spin REQUIRED)
find_package(Threads REQUIRED)
find_package(spdlog REQUIRED)

#
# source directory and files
#
set(SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src)
FILE(GLOB_RECURSE SRC_FILES CONFIGURE_DEPENDS ${SRC_DIR}/*.cpp)
list(REMOVE_ITEM SRC_FILES ${SRC_DIR}/main.cpp)
set(MODEL_SRC ${CMAKE_CURRENT_SOURCE_DIR}/src/network.pml)

#
# spin model target
#
add_library(spin_model STATIC ${CMAKE_CURRENT_BINARY_DIR}/model.c
            ${CMAKE_CURRENT_BINARY_DIR}/model.h)
target_include_directories(spin_model PRIVATE ${SRC_DIR})
target_include_directories(spin_model INTERFACE ${CMAKE_CURRENT_BINARY_DIR})
get_target_property(spin_compile_options spin_model COMPILE_OPTIONS)
list(REMOVE_ITEM spin_compile_options -Wall -Wextra -Werror)
set_target_properties(spin_model PROPERTIES COMPILE_OPTIONS "${spin_compile_options}")
add_custom_command(
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/model.c
    COMMAND sed ${CMAKE_CURRENT_BINARY_DIR}/pan.c
            -e "'1s/^/#define main(...) spin_main(__VA_ARGS__)\\n/'"
            -e "'1s/^/#define exit(...) verify_exit(__VA_ARGS__)\\n/'"
            > ${CMAKE_CURRENT_BINARY_DIR}/model.c
    DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/pan.c)
add_custom_command(
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/model.h
    COMMAND sed ${CMAKE_CURRENT_BINARY_DIR}/pan.h
            -e "'1s/^/#ifdef __cplusplus\\nextern \"C\" {\\n#endif\\n/'"
            -e "'$$s/$$/\\n#ifdef __cplusplus\\n}\\n#endif/'"
            > ${CMAKE_CURRENT_BINARY_DIR}/model.h
    DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/pan.h)
add_custom_command(
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/pan.c ${CMAKE_CURRENT_BINARY_DIR}/pan.h
    COMMAND ${Spin_EXECUTABLE} ${SPIN_FLAGS} ${MODEL_SRC}
    DEPENDS Spin::Spin ${MODEL_SRC})

#
# neo object library (libneo)
#
add_library(libneo OBJECT ${SRC_FILES})
target_include_directories(libneo PUBLIC ${SRC_DIR})
target_link_libraries(libneo PUBLIC coverage_config)
target_link_libraries(libneo PRIVATE
    CURL::libcurl
    Libnet::Libnet
    Libnl::Libnl
    PcapPlusPlus::pcap++
    Threads::Threads
    rapidjson
    spdlog::spdlog
    spin_model
    tomlplusplus::tomlplusplus
    xxHash::xxhash
)

#
# main target
#
add_executable(neo $<TARGET_OBJECTS:libneo> ${SRC_DIR}/main.cpp)
target_link_libraries(neo PRIVATE libneo)

#
# set main target capability
#
add_custom_target(setcap ALL
    sudo setcap "cap_sys_admin+pe cap_net_admin+pe cap_net_raw+pe cap_sys_ptrace+pe" neo
    DEPENDS neo)

#
# tests targets
#
option(ENABLE_TESTS "Build tests" OFF)
if (ENABLE_TESTS)
    enable_testing()
    add_subdirectory(test)
endif()


#
# install
#
install(TARGETS neo DESTINATION bin)
