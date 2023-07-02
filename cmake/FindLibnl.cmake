#[=======================================================================[.rst:
FindLibnl
----------

Finds the Libnl library.

Imported Targets
^^^^^^^^^^^^^^^^

This module provides the following imported targets, if found:

``Libnl::Libnl``
  The Libnl library

Result Variables
^^^^^^^^^^^^^^^^

This will define the following variables:

``Libnl_FOUND``
  True if the system has the Libnl library.
``Libnl_INCLUDE_DIRS``
  Include directories needed to use Libnl.
``Libnl_LIBRARIES``
  Libraries needed to link to Libnl.

#]=======================================================================]

set(LIBNL_VERSION 3.4.0)
find_package(PkgConfig REQUIRED)

pkg_check_modules(Libnl REQUIRED IMPORTED_TARGET
                  libnl-3.0>=${LIBNL_VERSION}
                  libnl-genl-3.0>=${LIBNL_VERSION})

if(Libnl_FOUND AND NOT TARGET Libnl::Libnl)
    add_library(Libnl::Libnl ALIAS PkgConfig::Libnl)
endif()
