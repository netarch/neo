#!/bin/bash
set -e

SCRIPT_DIR="$(dirname $(realpath ${BASH_SOURCE[0]}))"

${SCRIPT_DIR}/filename.sh
${SCRIPT_DIR}/astyle.sh
${SCRIPT_DIR}/build.sh -T
#${SCRIPT_DIR}/build.sh -Ta
#${SCRIPT_DIR}/build.sh -Tt
#${SCRIPT_DIR}/build.sh -Tc
