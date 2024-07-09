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
    -v, --verbose       Enable verbose test output
EOF
}

parse_args() {
    VERBOSE=0

    while :; do
        case "${1-}" in
        -h | --help)
            usage
            exit
            ;;
        -v | --verbose)
            VERBOSE=1
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

    # Prepare test parameters
    local ctest_flags=(
        --progress
        --output-on-failure
    )
    if [[ $VERBOSE -eq 1 ]]; then
        ctest_flags+=(--extra-verbose)
    fi

    # Run the tests
    cd "$BUILD_DIR"
    ctest "${ctest_flags[@]}"
}

main "$@"

# vim: set ts=4 sw=4 et:
