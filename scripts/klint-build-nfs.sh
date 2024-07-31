#!/usr/bin/env bash

set -euo pipefail

msg() {
    echo -e "[+] ${1-}" >&2
}

die() {
    echo -e "[!] ${1-}" >&2
    exit 1
}

[ $UID -eq 0 ] && die 'Please run this script without root privilege'

SCRIPT_DIR="$(dirname "$(realpath "${BASH_SOURCE[0]}")")"
PROJECT_DIR="$(dirname "$SCRIPT_DIR")"
BUILD_DIR="$PROJECT_DIR/build"
DOCKERFILES_DIR="$PROJECT_DIR/Dockerfiles"

check_depends() {
    for cmd in "$@"; do
        if ! command -v "$cmd" >/dev/null 2>&1; then
            die "'$cmd' not found"
        fi
    done
}

#
# Output a short name of the Linux distribution
#
get_distro() {
    if test -f /etc/os-release; then # freedesktop.org and systemd
        . /etc/os-release
        echo "$NAME" | cut -f 1 -d ' ' | tr '[:upper:]' '[:lower:]'
    elif type lsb_release >/dev/null 2>&1; then # linuxbase.org
        lsb_release -si | tr '[:upper:]' '[:lower:]'
    elif test -f /etc/lsb-release; then
        # shellcheck source=/dev/null
        source /etc/lsb-release
        echo "$DISTRIB_ID" | tr '[:upper:]' '[:lower:]'
    elif test -f /etc/arch-release; then
        echo "arch"
    elif test -f /etc/debian_version; then
        # Older Debian, Ubuntu
        echo "debian"
    elif test -f /etc/SuSe-release; then
        # Older SuSE
        echo "opensuse"
    elif test -f /etc/fedora-release; then
        # Older Fedora
        echo "fedora"
    elif test -f /etc/redhat-release; then
        # Older Red Hat, CentOS
        echo "centos"
    elif type uname >/dev/null 2>&1; then
        # Fall back to uname
        uname -s
    else
        die 'Unable to determine the distribution'
    fi
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
    check_depends git make meson ninja

    DISTRO="$(get_distro)"
    NFs=(firewall maglev nat faulty-nat)
    KLINT_URL=https://github.com/kyechou/klint.git
    KLINT_DIR="$BUILD_DIR/klint"
    KLINT_COMMIT=b2febb1d3f751b7c627edb0200e9594d9fdfa709

    if [ "$DISTRO" = "arch" ]; then
        LIB_DIR="/usr/lib"
    elif [ "$DISTRO" = "ubuntu" ]; then
        LIB_DIR="/usr/lib/x86_64-linux-gnu"
    else
        die "Unsupported distribution: $DISTRO"
    fi

    if [[ ! -e "$KLINT_DIR" ]]; then
        mkdir -p "$KLINT_DIR"
        cd "$KLINT_DIR"
        git init
        git remote add origin "$KLINT_URL"
        git fetch --depth 1 origin "$KLINT_COMMIT"
        git checkout FETCH_HEAD
    fi
    cd "$KLINT_DIR"

    for NF in "${NFs[@]}"; do
        msg "Building $NF..."
        make OS=dpdk NET=dpdk "build-$NF"
        make -j -C "$KLINT_DIR/env" \
            OS=dpdk NET=dpdk \
            NF="$KLINT_DIR/nf/$NF/libnf.so" \
            OS_CONFIG="$KLINT_DIR/env/config" \
            NF_CONFIG="$KLINT_DIR/nf/$NF/config"
        msg "Finished building $NF"

        NF_DIR="$DOCKERFILES_DIR/klint-$NF"
        msg "Installing $NF binaries and required shared libraries"
        install -Dm755 "$KLINT_DIR/env/build/bin" -t "$NF_DIR"
        install -D "$KLINT_DIR/nf/$NF/libnf.so" -t "$NF_DIR/$KLINT_DIR/nf/$NF"
        install -D "$LIB_DIR/libnuma.so.1" -t "$NF_DIR"
        install -D "$LIB_DIR/libc.so.6" -t "$NF_DIR"
        install -D "$LIB_DIR/libm.so.6" -t "$NF_DIR"
        install -D "$LIB_DIR/ld-linux-x86-64.so.2" -t "$NF_DIR"
    done
}

main "$@"

# vim: set ts=4 sw=4 et:
