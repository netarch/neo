#!/bin/bash

SCRIPT_DIR="$(dirname $(realpath ${BASH_SOURCE[0]}))"
source "${SCRIPT_DIR}/../common.sh"

build

sudo "$NEO" -f -i "$CONF" -o "$RESULTS_DIR"
