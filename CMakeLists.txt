cmake_minimum_required(VERSION 3.22)

#
# cmake configure options
#
option(BUILD_TESTS "Build tests" ON)

#
# vcpkg manifest mode
#
set(VCPKG_MANIFEST_DIR
    "${CMAKE_CURRENT_SOURCE_DIR}/depends"
    CACHE STRING "Path to vcpkg manifest directory")
if(BUILD_TESTS)
    list(APPEND VCPKG_MANIFEST_FEATURES "tests")
endif()
set(CMAKE_TOOLCHAIN_FILE
    "${CMAKE_CURRENT_SOURCE_DIR}/depends/vcpkg/scripts/buildsystems/vcpkg.cmake"
    CACHE STRING "Vcpkg toolchain file")

#
# project declaration
#
project(Neo VERSION 0.0.1 HOMEPAGE_URL https://github.com/netarch/neo LANGUAGES C CXX)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

#
# compile flags
#
set(CMAKE_C_STANDARD 17)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
# MAX_CONNS 32 => VECTORSZ > 3900
set(MAX_CONNS 32 CACHE STRING "Maximum number of communications")
set(VECTORSZ 4000 CACHE STRING "Memory allocation for spin state vector")
include("CheckTypeSize")
check_type_size("void *" SIZEOF_VOID_P)
check_type_size("int" SIZEOF_INT)
# NOTE: This `-O2` is crucial for preventing eBPF program verification errors.
add_compile_options(-Wall -Wextra -Werror -O2)

#
# release/debug compile flags
#
set(CMAKE_C_FLAGS_RELEASE          "-O3 -g")
set(CMAKE_CXX_FLAGS_RELEASE        "-O3 -g")
set(CMAKE_C_FLAGS_RELWITHDEBINFO   "-O2 -g")
set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "-O2 -g")
set(CMAKE_C_FLAGS_DEBUG   "-DENABLE_DEBUG -g3 -fsanitize=address -fno-omit-frame-pointer -fno-common")
set(CMAKE_CXX_FLAGS_DEBUG "-DENABLE_DEBUG -g3 -fsanitize=address -fno-omit-frame-pointer -fno-common")
set(CMAKE_EXE_LINKER_FLAGS_DEBUG "-fsanitize=address")
if("${CMAKE_BUILD_TYPE}" STREQUAL "")
    set(CMAKE_BUILD_TYPE RelWithDebInfo)
endif()

#
# third party libraries
#
include("ExternalProject")
list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")
add_subdirectory(third_party/bpftool EXCLUDE_FROM_ALL)
add_subdirectory(third_party/libbpf EXCLUDE_FROM_ALL)
add_subdirectory(third_party/libnet EXCLUDE_FROM_ALL)
find_package(boost_program_options CONFIG REQUIRED)
find_package(boost_stacktrace_backtrace CONFIG REQUIRED)
find_package(boost_stacktrace_basic CONFIG REQUIRED)
find_package(boost_stacktrace_noop CONFIG REQUIRED)
find_package(BpfObject REQUIRED)
find_package(CURL CONFIG REQUIRED)
find_package(Libnl REQUIRED)
find_package(RapidJSON CONFIG REQUIRED)
find_package(Spin REQUIRED)
find_package(Threads REQUIRED)
find_package(PcapPlusPlus CONFIG REQUIRED)
find_package(spdlog CONFIG REQUIRED)
find_package(TomlPlusPlus REQUIRED)
find_package(xxHash CONFIG REQUIRED)
find_package(ZLIB REQUIRED) # required by `FindBpfObject.cmake`

#
# stacktrace
# https://www.boost.org/doc/libs/master/doc/html/stacktrace/configuration_and_build.html
#
add_library(stacktrace_config INTERFACE)
target_compile_definitions(stacktrace_config INTERFACE BOOST_STACKTRACE_LINK)
if("${CMAKE_BUILD_TYPE}" STREQUAL "Debug")
    # libbacktrace is already linked statically with Boost::stacktrace_backtrace
    # through our patched vcpkg, so `-lbacktrace` is not needed.
    target_link_libraries(stacktrace_config INTERFACE Boost::stacktrace_backtrace dl)
else()
    target_link_libraries(stacktrace_config INTERFACE Boost::stacktrace_noop)
endif()

#
# source directory and files
#
set(SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src)

#
# spin model target
#
add_library(spin_model STATIC ${CMAKE_CURRENT_BINARY_DIR}/model.c
            ${CMAKE_CURRENT_BINARY_DIR}/model.h)
target_include_directories(spin_model PRIVATE ${SRC_DIR})
target_include_directories(spin_model INTERFACE ${CMAKE_CURRENT_BINARY_DIR})
# compile-time options: https://spinroot.com/spin/Man/Pan.html#B
target_compile_definitions(spin_model PUBLIC
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
set(MODEL_SRC ${CMAKE_CURRENT_SOURCE_DIR}/src/network.pml)
# spin run-time options: https://spinroot.com/spin/Man/Spin.html
set(SPIN_FLAGS -a
    -DMAX_CONNS=${MAX_CONNS}
    -DSIZEOF_VOID_P=${SIZEOF_VOID_P}
    -DSIZEOF_INT=${SIZEOF_INT})
add_custom_command(
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/pan.c ${CMAKE_CURRENT_BINARY_DIR}/pan.h
    COMMAND ${Spin_EXECUTABLE} ${SPIN_FLAGS} ${MODEL_SRC}
    DEPENDS Spin::Spin ${MODEL_SRC})

#
# libneo object library
#
FILE(GLOB_RECURSE SRC_FILES CONFIGURE_DEPENDS ${SRC_DIR}/*.cpp)
list(REMOVE_ITEM SRC_FILES ${SRC_DIR}/main.cpp)
add_library(libneo OBJECT ${SRC_FILES})
add_dependencies(libneo libnet)
target_compile_definitions(libneo PRIVATE
    MAX_CONNS=${MAX_CONNS}
)
target_include_directories(libneo PUBLIC ${SRC_DIR} ${libnet_INCLUDE_DIRS})
target_link_libraries(libneo PUBLIC
    stacktrace_config
    Threads::Threads
    Libnl::Libnl
)
target_link_libraries(libneo PRIVATE
    Boost::program_options
    CURL::libcurl
    ${libnet_LIBRARIES}
    PCAP::PCAP PcapPlusPlus::Pcap++ PcapPlusPlus::Common++ PcapPlusPlus::Packet++
    rapidjson
    spdlog::spdlog
    spin_model
    tomlplusplus::tomlplusplus
    xxHash::xxhash
)

#
# link libneo with bpf applications
#
FILE(GLOB_RECURSE BPF_SRC_FILES CONFIGURE_DEPENDS ${SRC_DIR}/*.bpf.c)
foreach(BPF_SRC_FILE ${BPF_SRC_FILES})
    get_filename_component(BPF_STEM ${BPF_SRC_FILE} NAME_WE)
    bpf_object(${BPF_STEM} ${BPF_STEM}.bpf.c)
    target_link_libraries(libneo PUBLIC ${BPF_STEM}_skel)
endforeach()

#
# main target
#
add_executable(neo $<TARGET_OBJECTS:libneo> ${SRC_DIR}/main.cpp)
target_link_libraries(neo PRIVATE libneo)

#
# set main target capability
#
# add_custom_target(setcap ALL
#    sudo setcap "cap_sys_admin+pe cap_net_admin+pe cap_net_raw+pe cap_sys_ptrace+pe" neo
#    DEPENDS neo)

#
# tests targets
#
if(BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

#
# install
#
install(TARGETS neo DESTINATION bin)
