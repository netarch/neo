#!/usr/bin/env bash

set -euo pipefail

SCRIPT_DIR="$(dirname "$(realpath "${BASH_SOURCE[0]}")")"
PROJECT_DIR="$(dirname "$SCRIPT_DIR")"
BUILD_DIR="${PROJECT_DIR}/build"

main() {
    export DESTDIR
    cmake --install "$BUILD_DIR" "$@"
}

main "$@"

# vim: set ts=4 sw=4 et:
