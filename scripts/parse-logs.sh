#!/usr/bin/env bash

set -euo pipefail

SCRIPT_DIR="$(dirname "$(realpath "${BASH_SOURCE[0]}")")"
PROJECT_DIR="$(dirname "$SCRIPT_DIR")"

msg() {
    echo -e "[+] ${1-}" >&2
}

die() {
    echo -e "[!] ${1-}" >&2
    exit 1
}

[ $UID -eq 0 ] && die 'Please run this script without root privilege'

main() {
    local targets=(
        "$PROJECT_DIR/examples/15-real-networks"
        "$PROJECT_DIR/examples/17-campus-network"
        "$PROJECT_DIR/examples/18-fat-tree-datacenter"
        "$PROJECT_DIR/full-emulation/00-reverse-path-filtering"
        "$PROJECT_DIR/full-emulation/15-real-networks"
        "$PROJECT_DIR/full-emulation/17-campus-network"
    )

    for target in "${targets[@]}"; do
        python3 "$SCRIPT_DIR/logparser.py" -t "$target"
    done
}

main "$@"

# vim: set ts=4 sw=4 et:
