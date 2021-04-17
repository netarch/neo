#!/bin/bash

SCRIPT_DIR="$(dirname $(realpath ${BASH_SOURCE[0]}))"
source "${SCRIPT_DIR}/../common.sh"

build

sudo "$NEO" -j12 -f -i "$CONF" -o "$RESULTS_DIR"
sudo chown -R $(id -u):$(id -g) "$RESULTS_DIR"
cp "$CONF" "$RESULTS_DIR"
