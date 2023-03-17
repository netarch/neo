#!/bin/bash

set -euo pipefail

SCRIPT_DIR="$(dirname "$(realpath "${BASH_SOURCE[0]}")")"
PROJECT_DIR="$(realpath "${SCRIPT_DIR}"/..)"
BUILD_DIR="$(realpath "${PROJECT_DIR}"/build)"
TEST_DIR="$(realpath "${PROJECT_DIR}"/test)"
MAKEFLAGS="-j$(nproc)"
export MAKEFLAGS

"$SCRIPT_DIR"/style.sh
"$SCRIPT_DIR"/build.sh --debug --tests --coverage --gcc
"$SCRIPT_DIR"/build.sh --debug --tests --coverage --clang

sudo "$BUILD_DIR/test/neotests" --test-data-dir "$TEST_DIR/networks" -d yes
