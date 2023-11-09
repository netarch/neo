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
DOCKERFILES_DIR="$PROJECT_DIR/Dockerfiles"

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
    check_depends git make meson ninja

    NFs=(bridge firewall maglev nat policer)
    KLINT_URL=https://github.com/kyechou/klint.git
    KLINT_DIR="$SCRIPT_DIR/klint"

    if [[ ! -e "$KLINT_DIR" ]]; then
        git clone --depth 1 "$KLINT_URL" "$KLINT_DIR"
    fi
    cd "$KLINT_DIR"

    for NF in "${NFs[@]}"; do
        msg "Building $NF..."
        make "build-$NF"
        make -j -C ./env OS=linux NET=dpdk NF="$KLINT_DIR/nf/$NF/libnf.so" \
            OS_CONFIG="$KLINT_DIR/env/config" \
            NF_CONFIG="$KLINT_DIR/nf/$NF/config"
        msg "Finished building $NF"

        NF_DIR="$DOCKERFILES_DIR/klint-$NF"
        msg "Installing $NF binaries and required shared libraries"
        install -Dm755 "$KLINT_DIR/env/build/bin" -t "$NF_DIR"
        install -D "$KLINT_DIR/nf/$NF/libnf.so" -t "$NF_DIR/$KLINT_DIR/nf/$NF"
        install -D /usr/lib/libnuma.so.1 -t "$NF_DIR"
        install -D /usr/lib/libc.so.6 -t "$NF_DIR"
        install -D /usr/lib/libm.so.6 -t "$NF_DIR"
        install -D /usr/lib/ld-linux-x86-64.so.2 -t "$NF_DIR"
    done
}

main "$@"

# vim: set ts=4 sw=4 et:
