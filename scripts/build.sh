#!/usr/bin/env bash

set -eo pipefail

SCRIPT_DIR="$(dirname "$(realpath "${BASH_SOURCE[0]}")")"
PROJECT_DIR="$(dirname "$SCRIPT_DIR")"
BUILD_DIR="${PROJECT_DIR}/build"

[ $UID -eq 0 ] && die 'Please run this script without root privilege'

cmake --build "$BUILD_DIR"

# vim: set ts=4 sw=4 et:
