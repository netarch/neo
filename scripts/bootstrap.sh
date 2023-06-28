#!/usr/bin/env bash

set -eo pipefail

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

usage() {
    cat <<EOF
[!] Usage: $(basename "${BASH_SOURCE[0]}") [options]

    Options:
    -h, --help          Print this message and exit
    -d, --debug         Enable debugging
    --compiler <...>    Either gcc or clang (default: clang)
EOF
}

parse_args() {
    BUILD_TYPE=Release
    COMPILER=clang

    while :; do
        case "${1-}" in
        -h | --help)
            usage
            exit
            ;;
        -d | --debug)
            BUILD_TYPE=Debug
            ;;
        --compiler)
            COMPILER="${2-}"
            if [[ "$COMPILER" != "clang" && "$COMPILER" != "gcc" ]]; then
                die "Unknown compiler: $COMPILER\n$(usage)"
            fi
            shift
            ;;
        -?*) die "Unknown option: $1\n$(usage)" ;;
        *) break ;;
        esac
        shift
    done

    SCRIPT_DIR="$(dirname "$(realpath "${BASH_SOURCE[0]}")")"
    PROJECT_DIR="$(dirname "$SCRIPT_DIR")"
    VENV_DIR="$PROJECT_DIR/.venv"
    BUILD_DIR="$PROJECT_DIR/build"
    GENERATORS_DIR="$BUILD_DIR/build/$BUILD_TYPE/generators"
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
    source "$GENERATORS_DIR/conanbuild.sh"
    # shellcheck source=/dev/null
    source "$GENERATORS_DIR/conanrun.sh"
}

deactivate_conan_env() {
    # shellcheck source=/dev/null
    source "$GENERATORS_DIR/deactivate_conanbuild.sh"
    # shellcheck source=/dev/null
    source "$GENERATORS_DIR/deactivate_conanrun.sh"
}

set_up_cpp_dependencies() {
    local recipe_path="$PROJECT_DIR/depends/conanfile.txt"
    local profile="$PROJECT_DIR/scripts/${COMPILER}_profile.ini"

    activate_python_venv
    conan install "$recipe_path" \
        --profile:host="$profile" \
        --profile:build="$profile" \
        --output-folder="$BUILD_DIR" \
        --settings=build_type="$BUILD_TYPE" \
        --build=missing \
        --build=cascade
    deactivate_python_venv
}

bootstrap() {
    parse_args "$@"
    set_up_python_dependencies
    set_up_cpp_dependencies
}

if [[ "$(basename "$0")" == "bootstrap.sh" ]]; then
    bootstrap "$@"
fi

# vim: set ts=4 sw=4 et:
