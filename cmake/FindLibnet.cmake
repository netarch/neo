#[=======================================================================[.rst:
FindLibnet
----------

Finds the Libnet library.

Imported Targets
^^^^^^^^^^^^^^^^

This module provides the following imported targets, if found:

``Libnet::Libnet``
  The Libnet library

Result Variables
^^^^^^^^^^^^^^^^

This will define the following variables:

``Libnet_FOUND``
  True if the system has the Libnet library.
``Libnet_VERSION``
  The version of the Libnet library which was found.
``Libnet_INCLUDE_DIRS``
  Include directories needed to use Libnet.
``Libnet_LIBRARIES``
  Libraries needed to link to Libnet.

#]=======================================================================]

find_path(Libnet_INCLUDE_DIRS NAMES libnet.h PATH_SUFFIXES libnet)
find_library(Libnet_LIBRARIES NAMES net)
set(Libnet_DEFINITIONS "-D_DEFAULT_SOURCE -DHAVE_NET_ETHERNET_H")
execute_process(COMMAND /bin/sh -c "grep LIBNET_VERSION ${Libnet_INCLUDE_DIRS}/libnet.h | grep -oE '[0-9]+\.[0-9]+\.[0-9]+'"
                OUTPUT_VARIABLE Libnet_VERSION
                OUTPUT_STRIP_TRAILING_WHITESPACE)
set(Libnet_VERSION_STRING ${Libnet_VERSION})

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Libnet
    FOUND_VAR Libnet_FOUND
    REQUIRED_VARS Libnet_INCLUDE_DIRS Libnet_LIBRARIES
    VERSION_VAR Libnet_VERSION)

if(Libnet_FOUND AND NOT TARGET Libnet::Libnet)
    add_library(Libnet::Libnet UNKNOWN IMPORTED)
    target_include_directories(Libnet::Libnet SYSTEM INTERFACE "${Libnet_INCLUDE_DIRS}")
    target_link_libraries(Libnet::Libnet INTERFACE "${Libnet_LIBRARIES}")
    target_compile_definitions(Libnet::Libnet INTERFACE "${Libnet_DEFINITIONS}")
    set_target_properties(Libnet::Libnet PROPERTIES IMPORTED_LOCATION "${Libnet_LIBRARIES}")
endif()
