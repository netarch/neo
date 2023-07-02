#!/usr/bin/env bash

set -eo pipefail

SCRIPT_DIR="$(dirname "$(realpath "${BASH_SOURCE[0]}")")"
PROJECT_DIR="$(dirname "$SCRIPT_DIR")"
BUILD_DIR="${PROJECT_DIR}/build"

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
    -j, --parallel N    Number of parallel build tasks
EOF
}

parse_args() {
    NUM_TASKS=$(nproc)

    while :; do
        case "${1-}" in
        -h | --help)
            usage
            exit
            ;;
        -j | --parallel)
            NUM_TASKS="${2-}"
            shift
            ;;
        -?*) die "Unknown option: $1\n$(usage)" ;;
        *) break ;;
        esac
        shift
    done
}

main() {
    # Parse script arguments
    parse_args "$@"
    # Activate the conan environment
    source "$SCRIPT_DIR/bootstrap.sh"
    activate_conan_env
    # Build
    cmake --build "$BUILD_DIR" -j "$NUM_TASKS"
}

main "$@"

# vim: set ts=4 sw=4 et:
