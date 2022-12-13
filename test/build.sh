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
    REBUILD=0

    while :; do
        case "${1-}" in
        -h | --help) usage; exit ;;
        -d | --debug)
            CMAKE_ARGS+=('-DCMAKE_BUILD_TYPE=Debug') ;;
        -r | --rebuild)
            REBUILD=1 ;;
        -t | --tests)
            CMAKE_ARGS+=('-DENABLE_TESTS=ON') ;;
        -c | --coverage)
            CMAKE_ARGS+=('-DENABLE_COVERAGE=ON') ;;
        --clang)
            CMAKE_ARGS+=('-DCMAKE_C_COMPILER=clang' '-DCMAKE_CXX_COMPILER=clang++') ;;
        --max-conns)
            CMAKE_ARGS+=("-DMAX_CONNS=${2-}")
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

    # fresh build
    cmake -B "${BUILD_DIR}" -S "${PROJECT_DIR}" "${CMAKE_ARGS[@]}"
    cmake --build "${BUILD_DIR}"
}


main "$@"

# vim: set ts=4 sw=4 et:
