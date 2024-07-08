#[=======================================================================[.rst:
FindTomlPlusPlus
----------------

Finds the tomlplusplus library.

Imported Targets
^^^^^^^^^^^^^^^^

This module provides the following imported targets, if found:

``tomlplusplus::tomlplusplus``
  The tomlplusplus library

Result Variables
^^^^^^^^^^^^^^^^

This will define the following variables:

``tomlplusplus_FOUND``
  True if the system has the tomlplusplus library.

#]=======================================================================]

find_package(PkgConfig REQUIRED)
pkg_check_modules(tomlplusplus REQUIRED IMPORTED_TARGET tomlplusplus)

if(tomlplusplus_FOUND AND NOT TARGET tomlplusplus::tomlplusplus)
    add_library(tomlplusplus::tomlplusplus ALIAS PkgConfig::tomlplusplus)
endif()
