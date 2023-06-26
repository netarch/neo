#!/usr/bin/env bash

set -eo pipefail

SCRIPT_DIR="$(dirname "$(realpath "${BASH_SOURCE[0]}")")"
PROJECT_DIR="$(dirname "$SCRIPT_DIR")"
BUILD_DIR="${PROJECT_DIR}/build"

die() {
    echo -e "[!] ${1-}" >&2
    exit 1
}

[ $UID -eq 0 ] && die 'Please run this script without root privilege'

usage() {
    cat <<EOF
[!] Usage: $(basename "${BASH_SOURCE[0]}") [options]

    Options:
    -h, --help          Print this message and exit
    -d, --debug         Enable debugging
    --clean             Clean all build files
    -r, --rebuild       Reconfigure and rebuild everything
    -t, --tests         Build tests
    -c, --coverage      Enable coverage
    --gcc               Use GCC
    --clang             Use Clang
    --max-conns N       Maximum number of concurrent connections
EOF
}

parse_args() {
    DEBUG=0
    CLEAN=0
    REBUILD=0
    TESTS=0
    COVERAGE=0
    COMPILER=clang
    MAX_CONNS=0

    while :; do
        case "${1-}" in
        -h | --help)
            usage
            exit
            ;;
        -d | --debug)
            DEBUG=1
            ;;
        --clean)
            CLEAN=1
            ;;
        -r | --rebuild)
            REBUILD=1
            ;;
        -t | --tests)
            TESTS=1
            ;;
        -c | --coverage)
            COVERAGE=1
            ;;
        --gcc)
            COMPILER=gcc
            ;;
        --clang)
            COMPILER=clang
            ;;
        --max-conns)
            MAX_CONNS="${2-}"
            shift
            ;;
        -?*) die "Unknown option: $1\n$(usage)" ;;
        *) break ;;
        esac
        shift
    done
}

prepare() {
    cd "$PROJECT_DIR"
    git submodule update --init --recursive

    local toolchain_file="$GENERATORS_DIR/conan_toolchain.cmake"
    CMAKE_ARGS=("-DCMAKE_TOOLCHAIN_FILE=$toolchain_file")
    PKG_CONFIG_PATH="/usr/lib/pkgconfig"
    MAKEFLAGS="-j$(nproc)"

    if [[ $DEBUG -ne 0 ]]; then
        CMAKE_ARGS+=('-DCMAKE_BUILD_TYPE=Debug')
    else
        CMAKE_ARGS+=('-DCMAKE_BUILD_TYPE=Release')
    fi
    if [[ $TESTS -ne 0 ]]; then
        CMAKE_ARGS+=('-DENABLE_TESTS=ON')
    else
        CMAKE_ARGS+=('-DENABLE_TESTS=OFF')
    fi
    if [[ $COVERAGE -ne 0 ]]; then
        CMAKE_ARGS+=('-DENABLE_COVERAGE=ON')
    else
        CMAKE_ARGS+=('-DENABLE_COVERAGE=OFF')
    fi
    if [[ "$COMPILER" = 'clang' ]]; then
        CMAKE_ARGS+=('-DCMAKE_C_COMPILER=clang' '-DCMAKE_CXX_COMPILER=clang++')
    elif [[ "$COMPILER" = 'gcc' ]]; then
        CMAKE_ARGS+=('-DCMAKE_C_COMPILER=gcc' '-DCMAKE_CXX_COMPILER=g++')
    fi
    if [[ "$MAX_CONNS" -ne 0 ]]; then
        CMAKE_ARGS+=("-DMAX_CONNS=${MAX_CONNS}")
    fi

    export CMAKE_ARGS
    export PKG_CONFIG_PATH
    export MAKEFLAGS
}

main() {
    # Bootstrap the python and conan environment
    source "$SCRIPT_DIR/bootstrap.sh"
    if [[ "$(bootstrapped)" == "false" ]]; then
        bootstrap
    fi

    # Activate the conan environment
    activate_conan_env
    # Parse script arguments
    parse_args "$@"
    # Prepare build files and parameters
    prepare

    if [[ $REBUILD -ne 0 ]] || [[ $CLEAN -ne 0 ]]; then
        # clean up old builds
        git submodule foreach --recursive git clean -xdf
        rm -rf "$BUILD_DIR"

        if [[ $CLEAN -ne 0 ]]; then
            exit 0
        fi
    fi

    # Configure and build
    cmake -B "$BUILD_DIR" -S "$PROJECT_DIR" "${CMAKE_ARGS[@]}"
    cmake --build "$BUILD_DIR"
}

main "$@"

# vim: set ts=4 sw=4 et:
