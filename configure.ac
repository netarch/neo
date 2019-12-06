##
## Package name, version, bug-report, package tarname, URL
##
AC_INIT([Neo], [0.0.1], [kychou2@illinois.edu], [neo],
        [https://github.com/netarch/neo])


##
## Check distribution
##
AS_IF([test -f /etc/os-release], [. /etc/os-release; OS=$(echo $NAME | cut -f 1 -d ' ')],
      [type lsb_release >/dev/null 2>&1], [OS=$(lsb_release -si)],
      [test -f /etc/lsb-release], [. /etc/lsb-release; OS=$DISTRIB_ID],
      [test -f /etc/arch-release], [OS="Arch"],
      [test -f /etc/debian_version], [OS="Debian"],
      [test -f /etc/SuSe-release], [OS="openSUSE"],
      [test -f /etc/fedora-release], [OS="Fedora"],
      [test -f /etc/redhat-release], [OS="CentOS"],
      [type uname >/dev/null 2>&1], [OS="$(uname -s)"],
      [AC_ERROR([unable to determine the distribution])])


##
## Determine license install path
##
AS_IF([test "$OS" = "Debian" -o "$OS" = "Ubuntu"],
      [AC_SUBST([licensedir], [\${docdir}])],
      [AC_SUBST([licensedir], [\${datarootdir}/licenses/\${PACKAGE_TARNAME}])])


##
## Config files and directories
##
AC_CONFIG_FILES([Makefile])
AC_CONFIG_FILES([codecov.sh:test/codecov.sh.in], [chmod +x codecov.sh])
AC_CONFIG_SRCDIR([src/network.pml])
AC_CONFIG_AUX_DIR([build-aux])


##
## Preset default flags
##
: ${CFLAGS="-O3"}
: ${CXXFLAGS="-O3"}
: ${LDFLAGS=""}
: ${LIBS=""}
: ${SPINFLAGS=""}


##
## Language environments
##
AC_LANG(C++)
AC_LANG_WERROR
AC_CHECK_SIZEOF([void *])
AC_CHECK_SIZEOF([int])


##
## Programs
##
AC_PROG_CC
AC_PROG_CXX
AC_PROG_LN_S
AC_PROG_INSTALL
AC_PROG_MKDIR_P


##
## Configure arguments
##
AC_ARG_ENABLE([coverage],
    AS_HELP_STRING([--enable-coverage], [Enable coverage testing using gcov]))
AC_ARG_ENABLE([asan],
    AS_HELP_STRING([--enable-asan], [Enable AddressSanitizer, LeakSanitizer,
                                     and UndefinedBehaviorSanitizer]))
AC_ARG_ENABLE([tsan],
    AS_HELP_STRING([--enable-tsan], [Enable ThreadSanitizer]))
AS_IF([test "$enable_asan" = "yes" -a "$enable_tsan" = "yes"], [
    AC_ERROR([ASan and TSan cannot be used at the same time])
])
AC_ARG_ENABLE([debug],
    AS_HELP_STRING([--enable-debug], [Enable debugging options]))


###
### Predefined functions
###
define([ADD_CPPFLAG], [
    AS_IF([test -z "$CPPFLAGS"], [CPPFLAGS="$1"],
          [AS_IF([test -n "${CPPFLAGS##*$1*}"], [CPPFLAGS="$CPPFLAGS $1"])])
])
define([ADD_CFLAG], [
    AS_IF([test -z "$CFLAGS"], [CFLAGS="$1"],
          [AS_IF([test -n "${CFLAGS##*$1*}"], [CFLAGS="$CFLAGS $1"])])
])
define([ADD_CXXFLAG], [
    AS_IF([test -z "$CXXFLAGS"], [CXXFLAGS="$1"],
          [AS_IF([test -n "${CXXFLAGS##*$1*}"], [CXXFLAGS="$CXXFLAGS $1"])])
])
define([ADD_CFLAG_CXXFLAG], [
    ADD_CFLAG([$1])
    ADD_CXXFLAG([$1])
])
define([ADD_LDFLAG], [
    AS_IF([test -z "$LDFLAGS"], [LDFLAGS="$1"],
          [AS_IF([test -n "${LDFLAGS##*$1*}"], [LDFLAGS="$LDFLAGS $1"])])
])
define([ADD_LIB], [
    AS_IF([test -z "$LIBS"], [LIBS="$1"],
          [AS_IF([test -n "${LIBS##*$1*}"], [LIBS="$LIBS $1"])])
])
define([ADD_SPINFLAG], [
    AS_IF([test -z "$SPINFLAGS"], [SPINFLAGS="$1"],
          [AS_IF([test -n "${SPINFLAGS##*$1*}"], [SPINFLAGS="$SPINFLAGS $1"])])
])
define([CHECK_CFLAGS], [
    AC_LANG_PUSH(C)
    AC_MSG_CHECKING([if c compiler flags "$1" work])
    save_CFLAGS="$CFLAGS"
    CFLAGS="$1"
    pushdef(prog, [int main(void) { return 0; }])
    flag_test=0
    AC_COMPILE_IFELSE([AC_LANG_SOURCE([prog])], [flag_test=1], [flag_test=0])
    popdef([prog])
    CFLAGS="$save_CFLAGS"
    if test $flag_test = 1
    then
        AC_MSG_RESULT(yes)
    else
        AC_MSG_RESULT(no)
        AC_ERROR([failed to compile with flags "$1"])
    fi
    AC_LANG_POP(C)
])
define([CHECK_CXXFLAGS], [
    AC_LANG_PUSH(C++)
    AC_MSG_CHECKING([if c++ compiler flags "$1" work])
    save_CXXFLAGS="$CXXFLAGS"
    CXXFLAGS="$1"
    pushdef(prog, [int main(void) { return 0; }])
    flag_test=0
    AC_COMPILE_IFELSE([AC_LANG_SOURCE([prog])], [flag_test=1], [flag_test=0])
    popdef([prog])
    CXXFLAGS="$save_CXXFLAGS"
    if test $flag_test = 1
    then
        AC_MSG_RESULT(yes)
    else
        AC_MSG_RESULT(no)
        AC_ERROR([failed to compile with flags "$1"])
    fi
    AC_LANG_POP(C++)
])
define([CHECK_LDFLAGS], [
    AC_MSG_CHECKING([if linker flags "$1" work])
    save_LDFLAGS="$LDFLAGS"
    LDFLAGS="$1"
    pushdef(prog, [int main(void) { return 0; }])
    flag_test=0
    AC_LINK_IFELSE([AC_LANG_SOURCE([prog])], [flag_test=1], [flag_test=0])
    popdef([prog])
    LDFLAGS="$save_LDFLAGS"
    if test $flag_test = 1
    then
        AC_MSG_RESULT(yes)
    else
        AC_MSG_RESULT(no)
        AC_ERROR([failed to link with flags "$1"])
    fi
])
define([CHECK_LIB], [
    save_LIBS="$LIBS"
    AC_CHECK_LIB($1, $2, $3, $4)
    LIBS="$save_LIBS"
])


##
## Compiler and linker flags
##
ADD_CFLAG([-std=c11])
ADD_CXXFLAG([-Wall -Wextra -Werror -std=c++17])
ADD_CPPFLAG([-iquote ${srcdir}/src -iquote .])
ADD_CPPFLAG([-isystem ${srcdir}/third_party])
ADD_SPINFLAG([-a])
ADD_SPINFLAG([-DNOBOUNDCHECK])
ADD_SPINFLAG([-DNOCOMP])
ADD_SPINFLAG([-DNOFAIR])
ADD_SPINFLAG([-DSAFETY])
ADD_SPINFLAG([-DSFH])
ADD_SPINFLAG([-DVECTORSZ=100])
ADD_SPINFLAG([-DSIZEOF_VOID_P="$ac_cv_sizeof_void_p"])
ADD_SPINFLAG([-DSIZEOF_INT="$ac_cv_sizeof_int"])
AS_IF([test "$enable_coverage" = "yes"], [
    CFLAGS="$(echo $CFLAGS | sed 's/-O[[0-9]]/-O0/g')"
    CXXFLAGS="$(echo $CXXFLAGS | sed 's/-O[[0-9]]/-O0/g')"
    ADD_CFLAG_CXXFLAG([--coverage])
    ADD_LIB(--coverage)
    AS_IF([test -z "$GCOV"], [      # if GCOV is not defined or null
           AS_IF([test -z "${CXX##*g++*}"], [GCOV="$(which gcov)"],
                 [test -z "${CXX##*clang++*}"], [GCOV="$(which llvm-cov) gcov"],
                 [AC_ERROR([unsupported compiler: $CXX])])],
          [test -n "${GCOV##/*}"], [        # else if GCOV is not an abs path
           GCOV_PATH="$(which $(echo $GCOV | cut -f 1 -d ' ') 2>/dev/null)"
           AS_IF([test -z "$GCOV_PATH"],
                 [AC_ERROR(["$(echo $GCOV | cut -f 1 -d ' ')" file not found])])
           GCOV="$(echo $GCOV | sed "s|[[^ ]]\+|$GCOV_PATH|")"])
    AC_MSG_CHECKING([for $(echo $GCOV | cut -f 1 -d ' ')])
    AS_IF([test -x "$(echo $GCOV | cut -f 1 -d ' ')"], [AC_MSG_RESULT(yes)], [
           AC_MSG_RESULT(no)
           AC_ERROR([$(echo $GCOV | cut -f 1 -d ' ') cannot be executed])])
    CXX_VER="$($CXX --version | grep -oE '[[0-9]]\.[[0-9]]\.[[0-9]]' | head -n1)"
    GCOV_VER="$($GCOV --version | grep -oE '[[0-9]]\.[[0-9]]\.[[0-9]]' | head -n1)"
    AS_IF([test "$CXX_VER" != "$GCOV_VER"],
          [AC_ERROR([$GCOV $GCOV_VER and $CXX $CXX_VER version mismatch])])
    AC_SUBST([GCOV], [$GCOV])
])
AS_IF([test "$enable_asan" = "yes"], [
    ADD_CFLAG_CXXFLAG(-fsanitize=address)
    ADD_CFLAG_CXXFLAG(-fno-omit-frame-pointer)
    ADD_CFLAG_CXXFLAG(-fno-common)
    ADD_LDFLAG(-fsanitize=address)
])
AS_IF([test "$enable_tsan" = "yes"], [
    ADD_CFLAG_CXXFLAG(-fsanitize=thread)
    ADD_LDFLAG(-fsanitize=thread)
])
AS_IF([test "$enable_debug" = "yes"], [
    CFLAGS="$(echo $CFLAGS | sed 's/-O[[0-9]]/-Og -g/g')"
    CXXFLAGS="$(echo $CXXFLAGS | sed 's/-O[[0-9]]/-Og -g/g')"
])


##
## Check flags usability
##
CHECK_CFLAGS([$CFLAGS])
CHECK_CXXFLAGS([$CXXFLAGS])
CHECK_LDFLAGS([$LDFLAGS])


##
## Libraries
##
AC_CHECK_HEADERS(libnet.h, [], [AC_ERROR([libnet is not installed])])
CHECK_LIB(net, libnet_init, [], [AC_ERROR([libnet does not work])])
ADD_LIB($(libnet-config --libs))

#AC_CHECK_HEADERS(pthread.h, [], [AC_ERROR([pthread is not available])])
#CHECK_LIB(pthread, pthread_create, [], [AC_ERROR([pthread does not work])])
#ADD_CFLAG(-pthread)
#ADD_LIB(-pthread)

#AC_DEFINE([PCRE2_CODE_UNIT_WIDTH], [8])
#AC_CHECK_HEADERS(pcre2.h, [], [AC_ERROR([pcre2 is not installed])])
#CHECK_LIB(pcre2-8, pcre2_compile_8, [], [AC_ERROR([pcre2 does not work])])
#ADD_LIB(-lpcre2-8)


echo >& AS_MESSAGE_FD
AC_SUBST([SPINFLAGS], [$SPINFLAGS])
AC_OUTPUT

# vim: set ts=4 sw=4 et:
