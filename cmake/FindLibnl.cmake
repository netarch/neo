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

# This is required as the PkgConfig module of Conan's cmake doesn't seem to
# search for system paths. We explicitly set the environment variable to search
# for libnl in the system. Once the libnl package is update in Conan's
# repository, we can remove the dependency on the system package.
set(ENV{PKG_CONFIG_PATH}
    "/usr/lib/pkgconfig:/usr/lib/x86_64-linux-gnu/pkgconfig")
pkg_check_modules(Libnl REQUIRED IMPORTED_TARGET
                  libnl-3.0>=${LIBNL_VERSION}
                  libnl-genl-3.0>=${LIBNL_VERSION})

if(Libnl_FOUND AND NOT TARGET Libnl::Libnl)
    add_library(Libnl::Libnl ALIAS PkgConfig::Libnl)
endif()
