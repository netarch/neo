#!/bin/bash
#
# Astyle check for source files.
#
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
    for cmd in $@; do
        if ! command -v $cmd >/dev/null 2>&1; then
            die "'$cmd' not found"
        fi
    done
}

usage() {
    cat <<EOF
[!] Usage: $(basename "${BASH_SOURCE[0]}") [options]

    Astyle check for source files.

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
    [ "$1" = "$2" ] && return 1 || verle $1 $2
}

main() {
    OVERWRITE=0
    parse_params $@
    check_depends astyle

    ASTYLE_OPTS=(
        '--suffix=none'
        '--formatted'
        '--style=kr'
        '--indent=spaces=4'
        '--indent-switches'
        '--pad-oper'
        '--pad-header'
        '--align-pointer=name'
        '--align-reference=type'
        '--lineend=linux'
    )
    ASTYLE_VER="$(astyle --version | head -n1 | cut -d ' ' -f 4)"
    if verle 2.06 $ASTYLE_VER; then
        ASTYLE_OPTS+=('--pad-comma')    # >= 2.06
    fi
    if verlt $ASTYLE_VER 3.0; then
        ASTYLE_OPTS+=('--add-brackets') # < 3.0
    else
        ASTYLE_OPTS+=('--add-braces')   # >= 3.0
    fi
    if [ $OVERWRITE -eq 0 ]; then
        ASTYLE_OPTS+=('--dry-run')
    fi

    SCRIPT_DIR="$(dirname $(realpath ${BASH_SOURCE[0]}))"
    SRC_DIR="$(realpath ${SCRIPT_DIR}/../src)"
    TEST_DIR="$(realpath ${SCRIPT_DIR}/../test)"
    FILES=$(find ${SRC_DIR} ${TEST_DIR} -type f | grep -E '\.(c|h|cpp|hpp)$')
    RESULT="$(astyle ${ASTYLE_OPTS[*]} ${FILES[*]})"

    if [ -n "$RESULT" ]; then
        if [ $OVERWRITE -eq 0 ]; then
            echo "$RESULT" | awk '{print $2}' | while read -r file; do
                hurt "$file style mismatch"
            done
            echo >&2
            die "Use: ${BASH_SOURCE[0]} -f"
        else
            echo "$RESULT" | while read -r line; do msg "$line"; done
        fi

        exit 1
    fi
}


main $@

# vim: set ts=4 sw=4 et:
