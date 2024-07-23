#!/usr/bin/env bash

set -euo pipefail

SCRIPT_DIR="$(dirname "$(realpath "${BASH_SOURCE[0]}")")"
PROJECT_DIR="$(dirname "$SCRIPT_DIR")"
BUILD_DIR="${PROJECT_DIR}/build"

msg() {
    echo -e "[+] ${1-}" >&2
}

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
    -t, --tests         Build tests
    --clean             Clean all build files without configuring
    --clang             Use Clang (default)
    --gcc               Use GCC
    --max-conns N       Maximum number of concurrent connections
EOF
}

parse_args() {
    DEBUG=0
    CLEAN=0
    TESTS=0
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
        -t | --tests)
            TESTS=1
            ;;
        --clean)
            CLEAN=1
            ;;
        --clang)
            COMPILER=clang
            ;;
        --gcc)
            COMPILER=gcc
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

reset_files() {
    local in_tree_submods=(
        "$PROJECT_DIR/third_party/bpftool/bpftool"
        "$PROJECT_DIR/third_party/libbpf/libbpf"
        "$PROJECT_DIR/third_party/libnet/libnet"
    )

    git -C "$PROJECT_DIR" submodule update --init --recursive
    for submod in "${in_tree_submods[@]}"; do
        msg "Cleaning $submod"
        git -C "$submod" restore .
        git -C "$submod" clean -xdf
    done

    local vcpkg_dir="$PROJECT_DIR/depends/vcpkg"
    msg "Patching $vcpkg_dir"
    git -C "$vcpkg_dir" restore .
    # git -C "$vcpkg_dir" clean -xdf # This will delete the build cache.
    local out
    out="$(patch -d "$vcpkg_dir" -Np1 -i "$PROJECT_DIR/depends/vcpkg.patch")" ||
        echo "$out" | grep -q 'Skipping patch' ||
        die "$out"

    msg "Removing $BUILD_DIR"
    rm -rf "$BUILD_DIR"

    if [[ $CLEAN -ne 0 ]]; then
        exit 0
    fi
}

prepare_flags() {
    CMAKE_ARGS=(
        "-DCMAKE_GENERATOR=Ninja"
        "-DCMAKE_MAKE_PROGRAM=ninja"
    )

    if [[ $DEBUG -ne 0 ]]; then
        CMAKE_ARGS+=('-DCMAKE_BUILD_TYPE=Debug')
    else
        CMAKE_ARGS+=('-DCMAKE_BUILD_TYPE=Release')
    fi
    if [[ $TESTS -ne 0 ]]; then
        CMAKE_ARGS+=('-DBUILD_TESTS=ON')
    else
        CMAKE_ARGS+=('-DBUILD_TESTS=OFF')
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
}

main() {
    parse_args "$@"
    reset_files
    prepare_flags
    cmake -B "$BUILD_DIR" -S "$PROJECT_DIR" "${CMAKE_ARGS[@]}"
}

main "$@"

# vim: set ts=4 sw=4 et:
