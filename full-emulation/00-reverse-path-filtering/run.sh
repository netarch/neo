#!/bin/bash

SCRIPT_DIR="$(dirname "$(realpath "${BASH_SOURCE[0]}")")"
EXAMPLE_DIR="$SCRIPT_DIR/../../examples/00-reverse-path-filtering"
export TOML_INPUT="$EXAMPLE_DIR/network.fault.toml"
source "$SCRIPT_DIR/../common.sh"
cd "$SCRIPT_DIR"

for i in {1..100}; do
    /usr/bin/time bash "$SCRIPT_DIR/single-run.sh" 2>&1 |
        tee "$RESULTS_DIR/$i.log"
done

msg "Done"
