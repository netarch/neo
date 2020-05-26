#!/bin/bash
set -e

SCRIPT_DIR="$(dirname $(realpath ${BASH_SOURCE[0]}))"

${SCRIPT_DIR}/filename.sh
${SCRIPT_DIR}/astyle.sh
${SCRIPT_DIR}/build.sh -T
