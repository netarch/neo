#!/bin/bash

SCRIPT_DIR="$(dirname "$(realpath "${BASH_SOURCE[0]}")")"
source "${SCRIPT_DIR}/../common.sh"

procs=1

drop="timeout"
name="output.$procs-procs.$drop"
run "$name" "$procs" "$drop" "$SCRIPT_DIR/network.toml"

# for drop in "${DROP_METHODS[@]}"; do
#     name="output.$procs-procs.$drop"
#     run "$name" "$procs" "$drop" "$SCRIPT_DIR/network.toml"
#     name="output.$procs-procs.$drop.fault"
#     run "$name" "$procs" "$drop" "$SCRIPT_DIR/network.fault.toml"
# done

msg "Done"
