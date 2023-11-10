#!/usr/bin/env bash

# This script should be run within a container.

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

usage() {
    cat <<EOF
[!] Usage: $(basename "${BASH_SOURCE[0]}") [options]

    Options:
    -h, --help          Print this message and exit
EOF
}

parse_params() {
    while :; do
        case "${1-}" in
        -h | --help)
            usage
            exit
            ;;
        -?*) die "Unknown option: $1\n$(usage)" ;;
        *) break ;;
        esac
        shift
    done
}

main() {
    parse_params "$@"
    check_depends curl tar meson ninja ldconfig

    VERSION=22.11.3
    DPDK_URL="https://fast.dpdk.org/rel/dpdk-$VERSION.tar.xz"
    DPDK_DIR="/dpdk"
    CONFIG=x86_64-native-linuxapp-gcc
    export CONFIG

    # Download
    cd /
    curl -LO $DPDK_URL
    tar xf dpdk-$VERSION.tar.xz
    mv "/dpdk-stable-$VERSION" "$DPDK_DIR"

    # Configure
    cd "$DPDK_DIR"
    meson setup -Dplatform=native build

    # Build
    cd "$DPDK_DIR/build"
    ninja

    # Install
    meson install
    ldconfig
}

main "$@"

# vim: set ts=4 sw=4 et:
