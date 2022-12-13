#!/bin/bash

set -euo pipefail

msg() {
    echo -e "[+] ${1-}" >&2
}

die() {
    echo -e "[!] ${1-}" >&2
    exit 1
}

check_depends() {
    for cmd in "$@"; do
        if ! command -v "$cmd" >/dev/null 2>&1; then
            die "'$cmd' not found"
        fi
    done
}

usage()
{
    cat <<EOF
[!] Usage: $(basename "${BASH_SOURCE[0]}") [options]

    Options:
    -h, --help          Print this message and exit
    -d, --debug         Enable debugging
    -r, --rebuild       Reconfigure and rebuild everything
    -t, --tests         Build tests
    -c, --coverage      Enable coverage
    --clang             Use Clang compiler
    --max-conns N       Maximum number of concurrent connections
EOF
}

parse_params() {
    DEBUG=0
    REBUILD=0
    TESTS=0
    COVERAGE=0
    CLANG=0
    MAX_CONNS=10

    while :; do
        case "${1-}" in
        -h | --help) usage; exit ;;
        -d | --debug)
            DEBUG=1 ;;
        -r | --rebuild)
            REBUILD=1 ;;
        -t | --tests)
            TESTS=1 ;;
        -c | --coverage)
            COVERAGE=1 ;;
        --clang)
            CLANG=1 ;;
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

main() {
    MAKEFLAGS="-j$(nproc)"
    CMAKE_ARGS=()
    parse_params "$@"
    check_depends cmake
    export MAKEFLAGS

    SCRIPT_DIR="$(dirname "$(realpath "${BASH_SOURCE[0]}")")"
    PROJECT_DIR="$(realpath "${SCRIPT_DIR}"/..)"
    BUILD_DIR="$(realpath "${PROJECT_DIR}"/build)"

    cd "${PROJECT_DIR}"
    git submodule update --init

    if [ $REBUILD -ne 0 ]; then
        # clean up old builds
        git submodule foreach --recursive git clean -xdf
        rm -rf "${BUILD_DIR}"
    fi

    if [ $DEBUG -ne 0 ]; then
        CMAKE_ARGS+=('-DCMAKE_BUILD_TYPE=Debug')
    else
        CMAKE_ARGS+=('-DCMAKE_BUILD_TYPE=Release')
    fi
    if [ $TESTS -ne 0 ]; then
        CMAKE_ARGS+=('-DENABLE_TESTS=ON')
    else
        CMAKE_ARGS+=('-DENABLE_TESTS=OFF')
    fi
    if [ $COVERAGE -ne 0 ]; then
        CMAKE_ARGS+=('-DENABLE_COVERAGE=ON')
    else
        CMAKE_ARGS+=('-DENABLE_COVERAGE=OFF')
    fi
    if [ $CLANG -ne 0 ]; then
        CMAKE_ARGS+=('-DCMAKE_C_COMPILER=clang' '-DCMAKE_CXX_COMPILER=clang++')
    fi
    CMAKE_ARGS+=("-DMAX_CONNS=${MAX_CONNS}")

    # configure and build
    cmake -B "${BUILD_DIR}" -S "${PROJECT_DIR}" "${CMAKE_ARGS[@]}"
    cmake --build "${BUILD_DIR}"
}


main "$@"

# vim: set ts=4 sw=4 et:
