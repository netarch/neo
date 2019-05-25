#!/bin/bash

type astyle >/dev/null 2>&1 || (echo '[!] astyle is not installed.' >&2; exit 1)

usage() {
    echo "[!] Usage: $0 [OPTIONS]" >&2
    echo '    Options:' >&2
    echo '        -a    automatically update the source files' >&2
    echo '        -h    show this message and exit' >&2
}

verle() {
    [ "$1" = "$(echo -e "$1\n$2" | sort -V | head -n1)" ]
}

verlt() {
    [ "$1" = "$2" ] && return 1 || verle $1 $2
}

AUTO=0
while getopts 'ah' op; do
    case $op in
        a)
            AUTO=1 ;;
        h|*)
            usage
            exit 1 ;;
    esac
done

EXITCODE=0
ASTYLE_VER="$(astyle --version | head -n1 | cut -d ' ' -f 4)"
ASTYLE_OPTS=(
    '--style=kr'
    '--indent=spaces=4'
    '--indent-switches'
    '--pad-oper'
    '--pad-header'
    '--align-pointer=name'
    '--align-reference=type'
    '--suffix=none'
)
if verle 2.06 $ASTYLE_VER; then
    ASTYLE_OPTS+=('--pad-comma')    # >= 2.06
fi
if verlt $ASTYLE_VER 3.0; then
    ASTYLE_OPTS+=('--add-brackets') # < 3.0
else
    ASTYLE_OPTS+=('--add-braces')   # >= 3.0
fi
SCRIPT_DIR="$(dirname $(realpath ${BASH_SOURCE[0]}))"
SRC_DIR="$(realpath ${SCRIPT_DIR}/../src)"
TEST_DIR="$(realpath ${SCRIPT_DIR}/../test)"
FILES=$(find ${SRC_DIR} ${TEST_DIR} -type f | grep -E '\.(c|h|cpp|hpp|pml)$')

for FILE in $FILES; do
    newfile="$(mktemp "tmp.XXXXXX")" || exit 1
    if ! astyle ${ASTYLE_OPTS[*]} <"$FILE" >"$newfile"; then
        echo >&2
        echo "[!] astyle failed." >&2
        exit 1
    fi
    if ! diff -upB "$FILE" "$newfile" &>/dev/null; then
        if [ $AUTO -ne 0 ]; then
            cp "$newfile" "$FILE"
            echo "[+] $FILE updated." >&2
        else
            echo "[-] $FILE style mismatch." >&2
        fi
        EXITCODE=1
    fi
    rm -f "$newfile"
done

if [ $AUTO -eq 0 -a $EXITCODE -eq 1 ]; then
    echo >&2
    echo "[!] Please run the following command:" >&2
    echo "    $SCRIPT_DIR/astyle.sh -a" >&2
fi

exit $EXITCODE
