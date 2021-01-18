#[=======================================================================[.rst:
FindSpin
----------

Finds Spin.

Imported Targets
^^^^^^^^^^^^^^^^

This module provides the following imported targets, if found:

``Spin::Spin``
  The Spin executable target

Result Variables
^^^^^^^^^^^^^^^^

This will define the following variables:

``Spin_FOUND``
  True if the system has Spin.
``Spin_VERSION``
  The version of Spin which was found.
``Spin_EXECUTABLE``
  Path to the Spin executable.

#]=======================================================================]

find_program(Spin_EXECUTABLE NAMES spin REQUIRED)
execute_process(COMMAND /bin/sh -c "${Spin_EXECUTABLE} -V | grep -oE '[0-9]+\.[0-9]+\.[0-9]+'"
                OUTPUT_VARIABLE Spin_VERSION
                OUTPUT_STRIP_TRAILING_WHITESPACE)
set(Spin_VERSION_STRING ${Spin_VERSION})

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Spin
    FOUND_VAR Spin_FOUND
    REQUIRED_VARS Spin_EXECUTABLE
    VERSION_VAR Spin_VERSION)

if(Spin_FOUND AND NOT TARGET Spin::Spin)
    add_executable(Spin::Spin IMPORTED GLOBAL)
    set_target_properties(Spin::Spin PROPERTIES IMPORTED_LOCATION "${Spin_EXECUTABLE}")
endif()
