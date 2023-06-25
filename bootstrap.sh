#!/usr/bin/env bash

set -euo pipefail

SCRIPT_DIR="$(dirname "$(realpath "${BASH_SOURCE[0]}")")"
cd "$SCRIPT_DIR"

msg() {
    echo -e "[+] ${1-}" >&2
}

die() {
    echo -e "[!] ${1-}" >&2
    exit 1
}

[ $UID -eq 0 ] && die 'Please run this script without root privilege'

check_depends() {
    for cmd in "$@"; do
        if ! command -v "$cmd" >/dev/null 2>&1; then
            die "'$cmd' not found"
        fi
    done
}

set_up_python_dependencies() {
    local venv_dir="$SCRIPT_DIR/.venv"
    local requirements_file="$SCRIPT_DIR/depends/requirements.txt"

    python3 -m venv "$venv_dir"
    # shellcheck source=/dev/null
    source "$venv_dir/bin/activate"
    python3 -m pip install --upgrade pip
    python3 -m pip install -r "$requirements_file"
    deactivate
}

main() {
    set_up_python_dependencies
}

main "$@"

# vim: set ts=4 sw=4 et:
