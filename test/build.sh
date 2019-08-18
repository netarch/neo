#!/bin/bash

usage()
{
    echo "[!] Usage: $0 [OPTIONS]" >&2
    echo '    Options:' >&2
    echo '        -T    enable unit testing' >&2
    echo '        -c    enable coverage testing' >&2
    echo '        -a    enable AddressSanitizer' >&2
    echo '        -t    enable ThreadSanitizer' >&2
    echo '        -d    enable debugging options' >&2
    echo '        -j    parallel build' >&2
    echo '        -h    show this message and exit' >&2
    echo '    Note:' >&2
    echo '        -a and -t cannot be used at the same time' >&2
}

add_config_flag()
{
    if [ -z "$CONFIG_FLAGS" ]; then
        CONFIG_FLAGS="$*"
    else
        CONFIG_FLAGS="$CONFIG_FLAGS $*"
    fi
}

add_makeflag()
{
    if [ -z "$MAKEFLAGS" ]; then
        MAKEFLAGS="$*"
    else
        MAKEFLAGS="$MAKEFLAGS $*"
    fi
}

TEST=0
COVERAGE=0
ASAN=0
TSAN=0
CONFIG_FLAGS=''
while getopts 'Tcatdjh' op; do
    case $op in
        T)
            TEST=1 ;;
        c)
            COVERAGE=1
            add_config_flag '--enable-coverage' ;;
        a)
            [ $TSAN -ne 0 ] && { usage; exit 1; }
            ASAN=1
            add_config_flag '--enable-asan' ;;
        t)
            [ $ASAN -ne 0 ] && { usage; exit 1; }
            TSAN=1
            add_config_flag '--enable-tsan' ;;
        d)
            add_config_flag '--enable-debug' ;;
        j)
            add_makeflag '-j' ;;
        h|*)
            usage
            exit 1 ;;
    esac
done

SCRIPT_DIR="$(dirname $(realpath ${BASH_SOURCE[0]}))"
PROJECT_DIR="$(realpath ${SCRIPT_DIR}/..)"

tmpdir="$(mktemp -d "$PROJECT_DIR/tmp.XXXXXX")" || {
    echo '[-] failed to create a temporary directory.' >&2
    exit 1
}

cd "${PROJECT_DIR}"
autoconf || {
    echo '[-] autoconf failed.' >&2
    rm -rf "$tmpdir"
    exit 1
}

EXITCODE=0
pushd "$tmpdir" >/dev/null 2>&1
for i in 0; do
    "${PROJECT_DIR}/configure" $CONFIG_FLAGS || {
        echo "[-] configure failed." >&2
        EXITCODE=1
        break
    }
    if [ $TEST -ne 0 ]; then
        make $MAKEFLAGS check || {
            echo '[-] make check failed.' >&2
            EXITCODE=1
            break
        }
        [ $COVERAGE -ne 0 ] && {
            ./codecov.sh || {
                echo '[-] codecov failed.' >&2
                EXITCODE=1
                break
            }
        }
    else
        make $MAKEFLAGS || {
            echo '[-] make failed.' >&2
            EXITCODE=1
            break
        }
    fi
done
popd >/dev/null 2>&1
rm -rf "$tmpdir" "${PROJECT_DIR}/autom4te.cache" "${PROJECT_DIR}/configure"
exit $EXITCODE
