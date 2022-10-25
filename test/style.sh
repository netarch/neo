#!/bin/bash

set -euo pipefail

msg() {
    echo -e "[+] ${1-}" >&2
}

hurt() {
    echo -e "[-] ${1-}" >&2
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
    -f, --overwrite     Overwrite the original source files
EOF
}

parse_params() {
    while :; do
        case "${1-}" in
        -h | --help) usage; exit ;;
        -f | --overwrite)
            OVERWRITE=1
            ;;
        -?*) die "Unknown option: $1\n$(usage)" ;;
        *) break ;;
        esac
        shift
    done
}

verle() {
    [ "$1" = "$(echo -e "$1\n$2" | sort -V | head -n1)" ]
}

verlt() {
    if [ "$1" = "$2" ]; then
        return 1
    else
        verle "$1" "$2"
    fi
}

main() {
    OVERWRITE=0
    parse_params "$@"
    check_depends clang-format

    SCRIPT_DIR="$(dirname "$(realpath "${BASH_SOURCE[0]}")")"
    SRC_DIR="$(realpath "${SCRIPT_DIR}"/../src)"
    TEST_DIR="$(realpath "${SCRIPT_DIR}"/../test)"

    if [ $OVERWRITE -eq 0 ]; then
        find "$SRC_DIR" "$TEST_DIR" -type f -regex '.*\.\(c\|h\|cpp\|hpp\)' \
            -exec clang-format --Werror --dry-run {} +
    else
        find "$SRC_DIR" "$TEST_DIR" -type f -regex '.*\.\(c\|h\|cpp\|hpp\)' \
            -exec clang-format --Werror -i {} +
    fi
}


main "$@"

# vim: set ts=4 sw=4 et:
