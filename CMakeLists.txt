cmake_minimum_required(VERSION 3.12)
project(Neo VERSION 0.1 HOMEPAGE_URL https://github.com/netarch/neo LANGUAGES C CXX)

# compile options
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(MAX_COMMS 2 CACHE STRING "Maximum number of communications")
include("CheckTypeSize")
check_type_size("void *" SIZEOF_VOID_P)
check_type_size("int" SIZEOF_INT)
add_compile_options(-O3 -g -Wall -Wextra -Werror)
add_compile_definitions(MAX_COMMS=${MAX_COMMS}
                        SIZEOF_VOID_P=${SIZEOF_VOID_P}
                        SIZEOF_INT=${SIZEOF_INT})
set(SPIN_FLAGS -a -DNOBOUNDCHECK -DNOCOMP -DNOFAIR -DSAFETY -DSFH -DVECTORSZ=100
    -DMAX_COMMS=${MAX_COMMS} -DSIZEOF_VOID_P=${SIZEOF_VOID_P}
    -DSIZEOF_INT=${SIZEOF_INT})

# debug compile options
set(CMAKE_C_FLAGS_DEBUG "-g3 -Og -fsanitize=address -fno-omit-frame-pointer -fno-common")
set(CMAKE_CXX_FLAGS_DEBUG "-g3 -Og -fsanitize=address -fno-omit-frame-pointer -fno-common")
set(CMAKE_EXE_LINKER_FLAGS_DEBUG "-fsanitize=address")

#TODO option(COVERAGE "Enable coverage testing" OFF)

# third party libraries
list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")
add_subdirectory(third_party/catch2)
add_subdirectory(third_party/cpptoml)
add_subdirectory(third_party/xxhash)
find_package(Libnet REQUIRED)
find_package(Spin REQUIRED)
find_package(Threads REQUIRED)

# source directory and files
set(SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src)
FILE(GLOB_RECURSE SRC_FILES CONFIGURE_DEPENDS ${SRC_DIR}/*.cpp)
set(MODEL_SRC ${CMAKE_CURRENT_SOURCE_DIR}/src/network.pml)

# spin model target
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

# main target
add_executable(neo ${SRC_FILES})
target_include_directories(neo PRIVATE ${SRC_DIR})
target_link_libraries(neo PRIVATE
    spin_model cpptoml xxhash Libnet::Libnet Threads::Threads)

# set capability
add_custom_target(setcap ALL
    sudo setcap "cap_sys_admin+pe cap_net_admin+pe cap_net_raw+pe cap_sys_ptrace+pe" neo
    DEPENDS neo)

# install
install(TARGETS neo DESTINATION bin)
