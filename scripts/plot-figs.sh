#!/usr/bin/env bash

set -euo pipefail

SCRIPT_DIR="$(dirname "$(realpath "${BASH_SOURCE[0]}")")"

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
        "00-reverse-path-filtering"
        # "03-load-balancing"
        "15-real-networks"
        "17-campus-network"
        "18-fat-tree-datacenter"
    )

    for target in "${targets[@]}"; do
        python3 "$SCRIPT_DIR/plot.py" -t "$target"
    done
}

main "$@"

# vim: set ts=4 sw=4 et:
