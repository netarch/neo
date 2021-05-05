cmake_minimum_required(VERSION 3.12)
project(Neo VERSION 0.1 HOMEPAGE_URL https://github.com/netarch/neo LANGUAGES C CXX)
include("CheckTypeSize")
include("ExternalProject")

#
# compile options
#
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(VECTORSZ 1024 CACHE STRING "Maximum number of communications")
set(MAX_CONNS 2 CACHE STRING "Maximum number of communications")
check_type_size("void *" SIZEOF_VOID_P)
check_type_size("int" SIZEOF_INT)
add_compile_options(-Wall -Wextra -Werror -O2)
if (CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    add_compile_options(-Wno-dynamic-class-memaccess -Wno-c99-designator)
elseif (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    if (CMAKE_CXX_COMPILER_VERSION VERSION_GREATER_EQUAL 10.0
            AND CMAKE_CXX_COMPILER_VERSION VERSION_LESS 11.0)
        add_compile_options(-Wno-stringop-overflow)
    endif()
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
set(CMAKE_C_FLAGS_DEBUG "-DENABLE_DEBUG -g -fsanitize=address -fno-omit-frame-pointer -fno-common")
set(CMAKE_CXX_FLAGS_DEBUG "-DENABLE_DEBUG -g -fsanitize=address -fno-omit-frame-pointer -fno-common")
set(CMAKE_EXE_LINKER_FLAGS_DEBUG "-fsanitize=address")

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
add_subdirectory(third_party/cpptoml)
add_subdirectory(third_party/spdlog)
add_subdirectory(third_party/xxhash)
find_package(Libnet REQUIRED)
find_package(Libnl REQUIRED)
find_package(Spin REQUIRED)
find_package(Threads REQUIRED)

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
add_dependencies(libneo spdlog)
target_include_directories(libneo PUBLIC ${SRC_DIR})
target_include_directories(libneo SYSTEM PRIVATE ${spdlog_INCLUDE_DIRS})
target_link_libraries(libneo PUBLIC coverage_config)
target_link_libraries(libneo PRIVATE spin_model cpptoml xxhash Libnet::Libnet
    Libnl::Libnl Threads::Threads)

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
