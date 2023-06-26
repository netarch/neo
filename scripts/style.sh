#!/usr/bin/env bash

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
    -f, --overwrite     Overwrite the original source files
EOF
}

parse_params() {
    while :; do
        case "${1-}" in
        -h | --help)
            usage
            exit
            ;;
        -f | --overwrite)
            OVERWRITE=1
            ;;
        -?*) die "Unknown option: $1\n$(usage)" ;;
        *) break ;;
        esac
        shift
    done
}

main() {
    OVERWRITE=0
    parse_params "$@"

    if command -v yapf3 >/dev/null 2>&1; then
        YAPF=yapf3
    else
        YAPF=yapf
    fi

    check_depends clang-format "$YAPF"

    SCRIPT_DIR="$(dirname "$(realpath "${BASH_SOURCE[0]}")")"
    SRC_DIR="$(realpath "${SCRIPT_DIR}"/../src)"
    TEST_DIR="$(realpath "${SCRIPT_DIR}"/../tests)"
    EXAMPLES_DIR="$(realpath "${SCRIPT_DIR}"/../examples)"
    TARGET_DIRS=("$SRC_DIR" "$TEST_DIR" "$EXAMPLES_DIR")

    if [[ $OVERWRITE -eq 0 ]]; then
        # C++: clang-format
        find "${TARGET_DIRS[@]}" -type f -regex '.*\.\(c\|h\|cpp\|hpp\)' \
            -exec clang-format --Werror --dry-run {} +
        # Python: yapf
        $YAPF -p -r -d "${TARGET_DIRS[@]}"
        msg "Coding style is compliant"
    else
        # C++: clang-format
        find "${TARGET_DIRS[@]}" -type f -regex '.*\.\(c\|h\|cpp\|hpp\)' \
            -exec clang-format --Werror -i {} +
        # Python: yapf
        $YAPF -p -r -i "${TARGET_DIRS[@]}"
    fi
}

main "$@"

# vim: set ts=4 sw=4 et:
