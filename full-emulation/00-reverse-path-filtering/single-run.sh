#!/bin/bash

SCRIPT_DIR="$(dirname "$(realpath "${BASH_SOURCE[0]}")")"
source "$SCRIPT_DIR/../common.sh"
cd "$SCRIPT_DIR"

startup &>/dev/null
set +e
# Packet tests
for i in {1..10}; do
    docker exec -it "h$i" hping3 10.0.0.1 -c 1 -i u100000 -n --icmp |
        grep -e "packets received"
done
set -e
get_container_memories
cleanup &>/dev/null
