#!/bin/bash

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

if [ $UID -eq 0 ]; then
    die 'Please run this script without root privilege'
fi

main() {
    git -C "$PROJECT_DIR" restore .
    patch -d "$PROJECT_DIR" -Np1 -i "$SCRIPT_DIR/unoptimized.patch"
    bash "$SCRIPT_DIR/configure.sh" -t
    bash "$SCRIPT_DIR/build.sh"

    cd "$PROJECT_DIR/examples/03-load-balancing.unopt/"
    ./run.sh
}

main "$@"

# vim: set ts=4 sw=4 et:
