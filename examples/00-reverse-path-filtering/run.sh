#!/bin/bash

SCRIPT_DIR="$(dirname "$(realpath "${BASH_SOURCE[0]}")")"
source "${SCRIPT_DIR}/../common.sh"

procs=16

drop="timeout"
# drop="dropmon"
# drop="ebpf"

name="output.$procs-procs.$drop"
run "$name" "$procs" "$drop" "$SCRIPT_DIR/network.toml"
name="output.$procs-procs.$drop.fault"
run "$name" "$procs" "$drop" "$SCRIPT_DIR/network.fault.toml"

msg "Done"
