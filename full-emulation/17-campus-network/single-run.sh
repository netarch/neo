#!/bin/bash

SCRIPT_DIR="$(dirname "$(realpath "${BASH_SOURCE[0]}")")"
source "$SCRIPT_DIR/../common.sh"
cd "$SCRIPT_DIR"

toml="$1"
network="$2"

# Prepare inputs: CONF, BRIDGES_TXT, DEVICES_TXT, INVS_TXT
"${CONFGEN[@]}" --network "$toml" >"$CONF"
"${CONFGEN[@]}" --network "$toml" --bridges >"$BRIDGES_TXT"
"${CONFGEN[@]}" --network "$toml" --devices >"$DEVICES_TXT"
"${CONFGEN[@]}" --network "$toml" --invariants >"$INVS_TXT"

startup &>/dev/null
mapfile -t invs <"$INVS_TXT"
for inv in "${invs[@]}"; do
    set +e
    # target_node,reachable,protocol,src_node,dst_ip
    IFS=, read -r _ reachable _ src_node dst_ip <<<"$inv"
    # Packet tests
    msg "Test $src_node -> $dst_ip: $reachable"
    docker exec -it "$src_node" hping3 "$dst_ip" -c 1 -i u100000 -n --icmp |
        grep -e "packets received"
    set -e
done
get_container_memories
cleanup &>/dev/null

# Save input files
mkdir -p "$RESULTS_DIR/$network"
mv "$CONF" "$BRIDGES_TXT" "$DEVICES_TXT" "$INVS_TXT" "$RESULTS_DIR/$network"
