#!/usr/bin/env bash

set -euo pipefail

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
EOF
}

parse_args() {
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
    SCRIPT_DIR="$(dirname "$(realpath "${BASH_SOURCE[0]}")")"
    PROJECT_DIR="$(dirname "$SCRIPT_DIR")"
    BUILD_DIR="${PROJECT_DIR}/build"

    # Parse script arguments
    parse_args "$@"

    # Run the tests
    sudo "$BUILD_DIR/tests/neotests" \
        --test-data-dir "$PROJECT_DIR/tests/networks" \
        --durations yes
}

main "$@"

# vim: set ts=4 sw=4 et:
