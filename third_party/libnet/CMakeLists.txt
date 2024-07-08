ExternalProject_Add(libnet
    PREFIX ${CMAKE_CURRENT_BINARY_DIR}
    SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/libnet
    CONFIGURE_COMMAND ./autogen.sh
    COMMAND ./configure
        --prefix=${CMAKE_CURRENT_BINARY_DIR}/install
        --disable-silent-rules
        --disable-shared
        --enable-static
        --disable-fast-install
        --disable-doxygen-doc
        --disable-doxygen-dot
        --disable-doxygen-man
        --disable-doxygen-rtf
        --disable-doxygen-xml
        --disable-doxygen-chm
        --disable-doxygen-chi
        --disable-doxygen-html
        --disable-doxygen-ps
        --disable-doxygen-pdf
        --disable-samples
        --enable-tests
        --with-pic
    BUILD_COMMAND make -s all install
    BUILD_BYPRODUCTS ${CMAKE_CURRENT_BINARY_DIR}/install/lib/libnet.a
    BUILD_IN_SOURCE TRUE
    INSTALL_COMMAND ""
    STEP_TARGETS build
)

set(libnet_VERSION "1.3" PARENT_SCOPE)
set(libnet_INCLUDE_DIRS ${CMAKE_CURRENT_BINARY_DIR}/install/include PARENT_SCOPE)
set(libnet_LIBRARIES ${CMAKE_CURRENT_BINARY_DIR}/install/lib/libnet.a PARENT_SCOPE)
