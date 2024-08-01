#!/bin/bash

SCRIPT_DIR="$(dirname "$(realpath "${BASH_SOURCE[0]}")")"
source "$SCRIPT_DIR/../common.sh"
cd "$SCRIPT_DIR"

EXAMPLE_DIR="$(realpath "$SCRIPT_DIR/../../examples/17-campus-network")"
for toml in "$EXAMPLE_DIR"/results/output.*/network.toml; do
    network="$(basename "$(dirname "$toml")" | sed -e 's/output\.//' -e 's/\.[0-9]\+-procs.*//')"
    mkdir -p "$RESULTS_DIR/$network"
    msg "Testing $network ..."
    /usr/bin/time bash "$SCRIPT_DIR/single-run.sh" "$toml" "$network" &>"$RESULTS_DIR/$network/out.log"
done

msg "Done"
