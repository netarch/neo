#!/usr/bin/env bash

set -euo pipefail

SCRIPT_DIR="$(dirname "$(realpath "${BASH_SOURCE[0]}")")"
PROJECT_DIR="$(dirname "$SCRIPT_DIR")"
VENV_DIR="$PROJECT_DIR/.venv"
BUILD_DIR="$PROJECT_DIR/build"

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

activate_python_venv() {
    # shellcheck source=/dev/null
    source "$VENV_DIR/bin/activate"
}

deactivate_python_venv() {
    deactivate
}

set_up_python_dependencies() {
    local requirements_file="$PROJECT_DIR/depends/requirements.txt"

    python3 -m venv --upgrade-deps "$VENV_DIR"
    activate_python_venv
    python3 -m pip install -r "$requirements_file"
    deactivate_python_venv
}

activate_conan_env() {
    # shellcheck source=/dev/null
    source "$BUILD_DIR/conanbuild.sh"
    # shellcheck source=/dev/null
    source "$BUILD_DIR/conanrun.sh"
}

deactivate_conan_env() {
    # shellcheck source=/dev/null
    source "$BUILD_DIR/deactivate_conanbuild.sh"
    # shellcheck source=/dev/null
    source "$BUILD_DIR/deactivate_conanrun.sh"
}

set_up_cpp_dependencies() {
    local recipe_path="$PROJECT_DIR/depends/conanfile.txt"
    local profile_path="$PROJECT_DIR/scripts/release_profile.ini"

    activate_python_venv
    conan install "$recipe_path" \
        --profile:host="$profile_path" \
        --profile:build="$profile_path" \
        --output-folder="$BUILD_DIR" \
        --build=missing \
        --build=cascade
    deactivate_python_venv
}

bootstrap() {
    set_up_python_dependencies
    set_up_cpp_dependencies

    # cmake .. -DCMAKE_TOOLCHAIN_FILE=$BUILD_DIR/conan_toolchain.cmake -DCMAKE_BUILD_TYPE=Release
    # cmake --build .
}

bootstrap "$@"

# vim: set ts=4 sw=4 et:
