#!/bin/bash
#
# Common shell utilities for experiment scripts
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

if [ $UID -eq 0 ]; then
    die 'Please run this script without root privilege'
fi

if [ -z "${SCRIPT_DIR+x}" ]; then
    die '"SCRIPT_DIR" is unset'
fi

export EXAMPLES_DIR="$(dirname $(realpath ${BASH_SOURCE[0]}))"
export PROJECT_DIR="$(realpath ${EXAMPLES_DIR}/..)"
export BUILD_DIR="$(realpath ${PROJECT_DIR}/build)"
export NEO="$BUILD_DIR/neo"
export SCRIPT_DIR
export CONF="${SCRIPT_DIR}/network.toml"
export CONFGEN=("python3" "${SCRIPT_DIR}/confgen.py")
export RESULTS_DIR="$(realpath ${SCRIPT_DIR}/results)"

mkdir -p "${RESULTS_DIR}"

build() {
    cmake -B "${BUILD_DIR}" -S "${PROJECT_DIR}" -DCMAKE_BUILD_TYPE=Release
    cmake --build "${BUILD_DIR}" -j$(nproc)
}
