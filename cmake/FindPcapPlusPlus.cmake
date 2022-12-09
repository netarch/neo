#[=======================================================================[.rst:
FindPcapPlusPlus
----------

Finds the PcapPlusPlus library.

Imported Targets
^^^^^^^^^^^^^^^^

This module provides the following imported targets, if found:

``PcapPlusPlus::pcap++``
  The PcapPlusPlus library

Result Variables
^^^^^^^^^^^^^^^^

This will define the following variables:

``PcapPlusPlus_FOUND``
  True if the system has the PcapPlusPlus library.
``PcapPlusPlus_INCLUDE_DIRS``
  Include directories needed to use PcapPlusPlus.
``PcapPlusPlus_LIBRARIES``
  Libraries needed to link to PcapPlusPlus.

#]=======================================================================]

set(PcapPlusPlus_VERSION 22.11)
find_package(PkgConfig REQUIRED)
pkg_check_modules(PcapPlusPlus REQUIRED IMPORTED_TARGET PcapPlusPlus)

if(PcapPlusPlus_FOUND AND NOT TARGET PcapPlusPlus::pcap++)
    add_library(PcapPlusPlus::pcap++ ALIAS PkgConfig::PcapPlusPlus)
endif()
