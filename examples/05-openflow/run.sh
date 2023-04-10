#!/bin/bash

SCRIPT_DIR="$(dirname "$(realpath "${BASH_SOURCE[0]}")")"
source "${SCRIPT_DIR}/../common.sh"

procs=16

for drop in "${DROP_METHODS[@]}"; do
    name="output.$procs-procs.$drop"
    run "$name" "$procs" "$drop" "$SCRIPT_DIR/network.toml"
done

msg "Done"
