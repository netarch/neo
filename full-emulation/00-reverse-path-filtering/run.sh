#!/bin/bash

SCRIPT_DIR="$(dirname "$(realpath "${BASH_SOURCE[0]}")")"
EXAMPLE_DIR="$SCRIPT_DIR/../../examples/00-reverse-path-filtering"
export TOML_INPUT="$EXAMPLE_DIR/network.fault.toml"
source "$SCRIPT_DIR/..//common.sh"

create_bridges
sudo containerlab deploy -t "$CONF"

# experiments
sleep 60

cleanup

msg "Done"
