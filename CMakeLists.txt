cmake_minimum_required(VERSION 3.10)
project(Neo VERSION 0.1 HOMEPAGE_URL https://github.com/netarch/neo LANGUAGES C CXX)
include("CheckTypeSize")
include("ExternalProject")

#
# compile options
#
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(MAX_COMMS 2 CACHE STRING "Maximum number of communications")
check_type_size("void *" SIZEOF_VOID_P)
check_type_size("int" SIZEOF_INT)
add_compile_options(-g -Wall -Wextra -Werror)
if (CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    add_compile_options(-Wno-dynamic-class-memaccess)
elseif (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    if (CMAKE_CXX_COMPILER_VERSION VERSION_GREATER_EQUAL 10.0
            AND CMAKE_CXX_COMPILER_VERSION VERSION_LESS 11.0)
        add_compile_options(-Wno-stringop-overflow)
    endif()
endif()
add_compile_definitions(MAX_COMMS=${MAX_COMMS}
                        SIZEOF_VOID_P=${SIZEOF_VOID_P}
                        SIZEOF_INT=${SIZEOF_INT})
set(SPIN_FLAGS -a -DNOBOUNDCHECK -DNOCOMP -DNOFAIR -DSAFETY -DSFH -DVECTORSZ=100
    -DMAX_COMMS=${MAX_COMMS} -DSIZEOF_VOID_P=${SIZEOF_VOID_P}
    -DSIZEOF_INT=${SIZEOF_INT})

#
# release/debug compile options
#
set(CMAKE_C_FLAGS_RELEASE "-O3")
set(CMAKE_CXX_FLAGS_RELEASE "-O3")
set(CMAKE_C_FLAGS_DEBUG "-DENABLE_DEBUG -g3 -Og -fsanitize=address -fno-omit-frame-pointer -fno-common")
set(CMAKE_CXX_FLAGS_DEBUG "-DENABLE_DEBUG -g3 -Og -fsanitize=address -fno-omit-frame-pointer -fno-common")
set(CMAKE_EXE_LINKER_FLAGS_DEBUG "-fsanitize=address")

#
# coverage options
#
option(ENABLE_COVERAGE "Enable coverage testing" OFF)
if (ENABLE_COVERAGE)
    add_compile_options(--coverage)
    #add_link_options()
endif()
#AS_IF([test "$enable_coverage" = "yes"], [
#    ADD_CFLAG_CXXFLAG([--coverage])
#    ADD_LIB(--coverage)
#    AS_IF([test -z "$GCOV"], [      # if GCOV is not defined or null
#           AS_IF([test -z "${CXX##*g++*}"], [GCOV="$(which gcov)"],
#                 [test -z "${CXX##*clang++*}"], [GCOV="$(which llvm-cov) gcov"],
#                 [AC_MSG_ERROR([unsupported compiler: $CXX])])],
#          [test -n "${GCOV##/*}"], [        # else if GCOV is not an abs path
#           GCOV_PATH="$(which $(echo $GCOV | cut -f 1 -d ' ') 2>/dev/null)"
#           AS_IF([test -z "$GCOV_PATH"],
#                 [AC_MSG_ERROR(["$(echo $GCOV | cut -f 1 -d ' ')" file not found])])
#           GCOV="$(echo $GCOV | sed "s|[[^ ]]\+|$GCOV_PATH|")"])
#    AC_MSG_CHECKING([for $(echo $GCOV | cut -f 1 -d ' ')])
#    AS_IF([test -x "$(echo $GCOV | cut -f 1 -d ' ')"], [AC_MSG_RESULT(yes)], [
#           AC_MSG_RESULT(no)
#           AC_MSG_ERROR([$(echo $GCOV | cut -f 1 -d ' ') cannot be executed])])
#    CXX_VER="$($CXX --version | grep -oE '[[0-9]]\.[[0-9]]\.[[0-9]]' | head -n1)"
#    GCOV_VER="$($GCOV --version | grep -oE '[[0-9]]\.[[0-9]]\.[[0-9]]' | head -n1)"
#    AS_IF([test "$CXX_VER" != "$GCOV_VER"],
#          [AC_MSG_ERROR([$GCOV $GCOV_VER and $CXX $CXX_VER version mismatch])])
#    AC_SUBST([GCOV], [$GCOV])
#])

#
# third party libraries
#
list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")
add_subdirectory(third_party/catch2)
add_subdirectory(third_party/cpptoml)
add_subdirectory(third_party/spdlog)
add_subdirectory(third_party/xxhash)
find_package(Libnet REQUIRED)
find_package(Spin REQUIRED)
find_package(Threads REQUIRED)

#
# source directory and files
#
set(SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src)
FILE(GLOB_RECURSE SRC_FILES CONFIGURE_DEPENDS ${SRC_DIR}/*.cpp)
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
# main target
#
add_executable(neo ${SRC_FILES})
add_dependencies(neo spdlog)
target_include_directories(neo PRIVATE ${SRC_DIR})
target_include_directories(neo SYSTEM PRIVATE ${spdlog_INCLUDE_DIRS})
target_link_libraries(neo PRIVATE spin_model cpptoml xxhash Libnet::Libnet
    Threads::Threads)

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
