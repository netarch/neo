#!/bin/bash

set -euo pipefail

SCRIPT_DIR="$(dirname $(realpath ${BASH_SOURCE[0]}))"
PROJECT_DIR="$(realpath ${SCRIPT_DIR}/..)"
BUILD_DIR="$(realpath ${PROJECT_DIR}/build)"
export MAKEFLAGS="-j$(nproc)"

${SCRIPT_DIR}/style.sh
${SCRIPT_DIR}/build.sh --debug --tests --coverage

#cd "${BUILD_DIR}"
#ctest
