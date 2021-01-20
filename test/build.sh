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

    Astyle check for source files.

    Options:
    -h, --help          Print this message and exit
    -d, --debug         Enable debugging
    -c, --coverage      Enable coverage
    -t, --unit-tests    Build unit tests
EOF
}

parse_params() {
    while :; do
        case "${1-}" in
        -h | --help) usage; exit ;;
        -d | --debug)
            CMAKE_ARGS+=('-DCMAKE_BUILD_TYPE=Debug') ;;
        -c | --coverage)
            CMAKE_ARGS+=('-DENABLE_COVERAGE=ON') ;;
        -t | --unit-tests)
            CMAKE_ARGS+=('-DENABLE_UNIT_TESTS=ON') ;;
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

    # clean up old builds
    git submodule foreach --recursive git clean -xdf
    rm -rf "${BUILD_DIR}"

    # fresh build
    cmake -B "${BUILD_DIR}" -S "${PROJECT_DIR}" ${CMAKE_ARGS[*]}
    make -C "${BUILD_DIR}"
}


main $@

# vim: set ts=4 sw=4 et:
