#!/bin/bash

SCRIPT_DIR="$(dirname "$(realpath "${BASH_SOURCE[0]}")")"
source "${SCRIPT_DIR}/../common.sh"

procs=16

name="output.$procs-procs"
run_with_timeout "$name" "$procs" "$SCRIPT_DIR/network.toml"
name="output.$procs-procs.fault"
run_with_timeout "$name" "$procs" "$SCRIPT_DIR/network.fault.toml"

msg "Done"
