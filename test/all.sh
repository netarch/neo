#!/bin/bash
set -euo pipefail

SCRIPT_DIR="$(dirname $(realpath ${BASH_SOURCE[0]}))"

${SCRIPT_DIR}/astyle.sh
${SCRIPT_DIR}/build.sh --debug --tests --coverage
