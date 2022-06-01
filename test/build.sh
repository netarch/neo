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
    for cmd in $@; do
        if ! command -v $cmd >/dev/null 2>&1; then
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
    -t, --tests         Build tests
    -c, --coverage      Enable coverage
    --clang             Use Clang compiler
    --max-comms N       Maximum number of concurrent communications
EOF
}

parse_params() {
    while :; do
        case "${1-}" in
        -h | --help) usage; exit ;;
        -d | --debug)
            CMAKE_ARGS+=('-DCMAKE_BUILD_TYPE=Debug') ;;
        -t | --tests)
            CMAKE_ARGS+=('-DENABLE_TESTS=ON') ;;
        -c | --coverage)
            CMAKE_ARGS+=('-DENABLE_COVERAGE=ON') ;;
        --clang)
            CMAKE_ARGS+=('-DCMAKE_C_COMPILER=clang' '-DCMAKE_CXX_COMPILER=clang++') ;;
        --max-comms)
            CMAKE_ARGS+=("-DMAX_COMMS=${2-}")
            shift
            ;;
        -?*) die "Unknown option: $1\n$(usage)" ;;
        *) break ;;
        esac
        shift
    done
}

main() {
    export MAKEFLAGS="-j$(nproc)"
    CMAKE_ARGS=()
    parse_params $@
    check_depends cmake

    SCRIPT_DIR="$(dirname $(realpath ${BASH_SOURCE[0]}))"
    PROJECT_DIR="$(realpath ${SCRIPT_DIR}/..)"
    BUILD_DIR="$(realpath ${PROJECT_DIR}/build)"

    cd "${PROJECT_DIR}"
    git submodule update --init

    # clean up old builds
    git submodule foreach --recursive git clean -xdf
    rm -rf "${BUILD_DIR}"

    # fresh build
    cmake -B "${BUILD_DIR}" -S "${PROJECT_DIR}" ${CMAKE_ARGS[*]}
    cmake --build "${BUILD_DIR}"
}


main $@

# vim: set ts=4 sw=4 et:
