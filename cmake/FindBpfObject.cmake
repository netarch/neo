#[=======================================================================[.rst:
FindBpfObject
----------

Find BpfObject

This module finds if all the dependencies for eBPF Compile-Once-Run-Everywhere
programs are available and where all the components are located.

This will define the following variables:

``BpfObject_FOUND``
  True if all components are found.

This module also provides the ``bpf_object()`` macro. This macro generates a
cmake interface library for the BPF object's generated skeleton as well
as the associated dependencies.

.. code-block:: cmake

  bpf_object(<name> <source>)

Given an abstract ``<name>`` for a BPF object and the associated ``<source>``
file, generates an interface library target, ``<name>_skel``, that may be
linked against by other cmake targets.

Example Usage:

::

  find_package(BpfObject REQUIRED)
  bpf_object(myobject myobject.bpf.c)
  add_executable(myapp myapp.c)
  target_link_libraries(myapp myobject_skel)

#]=======================================================================]

#
# Find clang and check clang version
#
find_program(CLANG_EXE NAMES clang REQUIRED)

execute_process(COMMAND ${CLANG_EXE} --version
    OUTPUT_VARIABLE CLANG_version_output
    ERROR_VARIABLE CLANG_version_error
    RESULT_VARIABLE CLANG_version_result
    OUTPUT_STRIP_TRAILING_WHITESPACE)

if (${CLANG_version_result} EQUAL 0)
    if ("${CLANG_version_output}" MATCHES "clang version ([^\n]+)\n")
        # Transform X.Y.Z into X;Y;Z which can then be interpreted as a list
        set(CLANG_VERSION "${CMAKE_MATCH_1}")
        string(REPLACE "." ";" CLANG_VERSION_LIST ${CLANG_VERSION})
        list(GET CLANG_VERSION_LIST 0 CLANG_VERSION_MAJOR)

        # Anything older than clang 10 doesn't really work
        string(COMPARE LESS ${CLANG_VERSION_MAJOR} 10 CLANG_VERSION_MAJOR_LT10)
        if (${CLANG_VERSION_MAJOR_LT10})
            message(FATAL_ERROR "clang ${CLANG_VERSION} is too old for BPF CO-RE")
        endif()

        message(STATUS "Found clang version: ${CLANG_VERSION}")
    else()
        message(FATAL_ERROR "Failed to parse clang version string: ${CLANG_version_output}")
    endif()
else()
    message(FATAL_ERROR "Command \"${CLANG_EXE} --version\" failed with output:\n${CLANG_version_error}")
endif()

#
# Get clang bpf system includes
#
execute_process(
    COMMAND bash -c "${CLANG_EXE} -v -E - < /dev/null 2>&1 |
            sed -n '/<...> search starts here:/,/End of search list./{ s| \\(/.*\\)|-idirafter \\1|p }'"
    OUTPUT_VARIABLE CLANG_SYSTEM_INCLUDES_output
    ERROR_VARIABLE CLANG_SYSTEM_INCLUDES_error
    RESULT_VARIABLE CLANG_SYSTEM_INCLUDES_result
    OUTPUT_STRIP_TRAILING_WHITESPACE)
if (${CLANG_SYSTEM_INCLUDES_result} EQUAL 0)
    separate_arguments(CLANG_SYSTEM_INCLUDES UNIX_COMMAND ${CLANG_SYSTEM_INCLUDES_output})
    message(STATUS "BPF system include flags: ${CLANG_SYSTEM_INCLUDES}")
else()
    message(FATAL_ERROR "Failed to determine BPF system includes: ${CLANG_SYSTEM_INCLUDES_error}")
endif()

#
# Generate vmlinux.h
#
set(VMLINUX_H ${CMAKE_CURRENT_BINARY_DIR}/vmlinux.h)
add_custom_command(OUTPUT ${VMLINUX_H}
    COMMAND ${bpftool_EXECUTABLE} btf dump file /sys/kernel/btf/vmlinux format c > ${VMLINUX_H}
    DEPENDS bpftool
    COMMENT "Generating vmlinux.h")

#
# Get target arch
#
execute_process(COMMAND uname -m
    COMMAND sed -e "s/x86_64/x86/" -e "s/aarch64/arm64/" -e "s/ppc64le/powerpc/" -e "s/mips.*/mips/"
    OUTPUT_VARIABLE ARCH_output
    ERROR_VARIABLE ARCH_error
    RESULT_VARIABLE ARCH_result
    OUTPUT_STRIP_TRAILING_WHITESPACE)
if (${ARCH_result} EQUAL 0)
    set(ARCH ${ARCH_output})
    message(STATUS "BPF target arch: ${ARCH}")
else()
    message(FATAL_ERROR "Failed to determine target architecture: ${ARCH_error}")
endif()

#
# Get compilation flags
#
get_directory_property(COMPILE_FLAGS COMPILE_OPTIONS)
list(REMOVE_ITEM COMPILE_FLAGS -Wno-maybe-uninitialized)

#
# Report dependencies found
#
include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(BpfObject
    FOUND_VAR BpfObject_FOUND
    REQUIRED_VARS
        CLANG_EXE
        CLANG_SYSTEM_INCLUDES
        COMPILE_FLAGS
        libbpf_INCLUDE_DIRS
        libbpf_LIBRARIES
        VMLINUX_H)

#
# Public macro
#
macro(bpf_object name input)
    set(BPF_C_FILE ${SRC_DIR}/${input})
    set(BPF_O_FILE ${CMAKE_CURRENT_BINARY_DIR}/${name}.bpf.o)
    set(BPF_SKEL_FILE ${CMAKE_CURRENT_BINARY_DIR}/${name}.skel.h)
    set(OUTPUT_TARGET ${name}_skel)

    # Build BPF object file
    add_custom_command(OUTPUT ${BPF_O_FILE}
        COMMAND ${CLANG_EXE} ${COMPILE_FLAGS} -g -target bpf
            -D__TARGET_ARCH_${ARCH} ${CLANG_SYSTEM_INCLUDES} -I${SRC_DIR}
            -I${CMAKE_CURRENT_BINARY_DIR} -isystem ${libbpf_INCLUDE_DIRS}
            -c ${BPF_C_FILE} -o ${BPF_O_FILE}
        MAIN_DEPENDENCY ${BPF_C_FILE}
        DEPENDS ${CLANG_EXE} ${BPF_C_FILE} ${VMLINUX_H}
        COMMENT "Building BPF object: ${name}")

    # Build BPF skeleton header
    add_custom_command(OUTPUT ${BPF_SKEL_FILE}
        COMMAND ${bpftool_EXECUTABLE} gen skeleton ${BPF_O_FILE} > ${BPF_SKEL_FILE}
        MAIN_DEPENDENCY ${BPF_O_FILE}
        DEPENDS bpftool ${BPF_O_FILE}
        COMMENT "Building BPF skeleton: ${name}")

    add_library(${OUTPUT_TARGET} INTERFACE)
    add_dependencies(${OUTPUT_TARGET} libbpf)
    target_sources(${OUTPUT_TARGET} INTERFACE ${BPF_SKEL_FILE})
    target_include_directories(${OUTPUT_TARGET} INTERFACE ${CMAKE_CURRENT_BINARY_DIR})
    target_include_directories(${OUTPUT_TARGET} SYSTEM INTERFACE ${libbpf_INCLUDE_DIRS})
    target_link_libraries(${OUTPUT_TARGET} INTERFACE ${libbpf_LIBRARIES} -lelf ZLIB::ZLIB)
endmacro()
