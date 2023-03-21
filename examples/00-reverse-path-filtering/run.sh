#!/bin/bash

SCRIPT_DIR="$(dirname "$(realpath "${BASH_SOURCE[0]}")")"
source "${SCRIPT_DIR}/../common.sh"

procs=16

# timeout
name="output.parallel-$procs.fault"
msg "Verifying $name"
sudo "$NEO" -f -j $procs -i "$CONF" -o "$RESULTS_DIR/$name"
sudo chown -R "$(id -u):$(id -g)" "$RESULTS_DIR/$name"
cp "$CONF" "$RESULTS_DIR/$name"

msg "Done"
