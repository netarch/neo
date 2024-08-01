#!/bin/bash

SCRIPT_DIR="$(dirname "$(realpath "${BASH_SOURCE[0]}")")"
EXAMPLE_DIR="$SCRIPT_DIR/../../examples/00-reverse-path-filtering"
export TOML_INPUT="$EXAMPLE_DIR/network.fault.toml"
source "$SCRIPT_DIR/../common.sh"
cd "$SCRIPT_DIR"

startup &>/dev/null
set +e
# Packet tests
for i in {1..10}; do
    docker exec -it "h$i" hping3 10.0.0.1 -c 1 -i u100000 -n --icmp |
        grep -e "packets received"
done
get_container_memories
set -e
cleanup &>/dev/null
