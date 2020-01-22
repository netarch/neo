#!/bin/bash

set -e
set -o nounset

#SCRIPT_DIR="$(dirname $(realpath ${BASH_SOURCE[0]}))"
#cd "$SCRIPT_DIR"

[ $UID -eq 0 ] && \
    (echo '[!] Please run this script without root privilege' >&2; exit 1)

[ $# -eq 0 ] && \
    (echo "[!] Usage: $0 <dir>" >&2; exit 1)

OUT_DIR="$(realpath "$*")"

[ ! -d "$OUT_DIR" ] && \
    (echo "[!] Directory not found: $OUT_DIR" >&2; exit 1)

for dir in "$OUT_DIR"/*; do
    [ ! -d "$dir" ] && continue

    num_subnets=$(basename "$dir" | cut -d '.' -f 1 | sed 's/-subnets//')
    num_hosts=$(basename "$dir" | cut -d '.' -f 2 | sed 's/-hosts//')
    DOP=$(basename "$dir" | cut -d '.' -f 3 | sed 's/DOP-//')
    MAIN_LOG="$dir"/main.log
    num_nodes=$(grep -E 'Loaded [0-9]+ nodes' "$MAIN_LOG" | grep -oE '[0-9]+')
    num_links=$(grep -E 'Loaded [0-9]+ links' "$MAIN_LOG" | grep -oE '[0-9]+')

    for policy in {1..7}; do
        result="$(grep -A 2 -E "$policy\. Verifying .+$" "$MAIN_LOG")"
        num_ECs=$(echo "$result" | grep 'Packet ECs:' | grep -oE '[0-9]+')
        policy_time=$(echo "$result" | grep 'Finished in' | grep -oE '[0-9]+')

        # output per-policy results
        echo $num_subnets,$num_hosts,$DOP,$num_nodes,$num_links,$policy,$num_ECs,$policy_time
    done

    total_time=$(tail -n2 "$MAIN_LOG" | head -n1 | grep -oE '[0-9]+')
    peak_mem=$(tail -n1 "$MAIN_LOG" | grep -oE '[0-9]+')

    # output per-network-setting results
    echo $num_subnets,$num_hosts,$DOP,$num_nodes,$num_links,1-7,N/A,$total_time,$peak_mem
done
