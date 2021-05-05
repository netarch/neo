#!/bin/bash

SCRIPT_DIR="$(dirname $(realpath ${BASH_SOURCE[0]}))"
source "${SCRIPT_DIR}/../common.sh"

build

procs=16

# timeout
name="rpf.DOP-$procs.fault"
msg "Verifying $name"
sudo "$NEO" -fj$procs -i "$CONF" -o "$RESULTS_DIR/$name"
sudo chown -R $(id -u):$(id -g) "$RESULTS_DIR/$name"
cp "$CONF" "$RESULTS_DIR/$name"
# dropmon
name="rpf.DOP-$procs.fault.dropmon"
msg "Verifying $name"
sudo "$NEO" -fdj$procs -i "$CONF" -o "$RESULTS_DIR/$name"
sudo chown -R $(id -u):$(id -g) "$RESULTS_DIR/$name"
cp "$CONF" "$RESULTS_DIR/$name"

msg "Populating measurements..."
extract_stats "$RESULTS_DIR" > "$RESULTS_DIR/stats.csv"
extract_latency "$RESULTS_DIR" > "$RESULTS_DIR/latency.csv"

msg "Done"
