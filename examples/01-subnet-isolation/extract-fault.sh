#!/bin/bash

set -e
set -o nounset

[ $UID -eq 0 ] && \
    (echo '[!] Please run this script without root privilege' >&2; exit 1)

[ $# -eq 0 ] && \
    (echo "[!] Usage: $0 <dir>" >&2; exit 1)

OUT_DIR="$(realpath "$*")"

[ ! -d "$OUT_DIR" ] && \
    (echo "[!] Directory not found: $OUT_DIR" >&2; exit 1)

echo '# of subnets, # of hosts, # of concurrent processes, # of nodes, # of links, Policy, # of ECs, # of communications, Time (microseconds), Memory (kilobytes)'

for dir in "$OUT_DIR"/*; do
    [ ! -d "$dir" -o "$dir" == *".latency" ] && continue

    num_subnets=$(basename "$dir" | cut -d '.' -f 1 | sed 's/-subnets//')
    num_hosts=$(basename "$dir" | cut -d '.' -f 2 | sed 's/-hosts//')
    DOP=$(basename "$dir" | cut -d '.' -f 3 | sed 's/DOP-//')

    for policy in "$dir"/*; do
        [ ! -d "$policy" ] && continue

        # output per-policy results
        policy_result="$(tail -n1 "$policy/stats.csv")"
        echo $num_subnets,$num_hosts,$DOP,$policy_result
    done

    ## output per-network-setting results
    #MAIN_LOG="$dir"/main.log
    #num_nodes=$(head -n2 "$MAIN_LOG" | grep -E 'Loaded [0-9]+ nodes' | grep -oE '[0-9]+')
    #num_links=$(head -n2 "$MAIN_LOG" | grep -E 'Loaded [0-9]+ links' | grep -oE '[0-9]+')
    #total_time=$(tail -n2 "$MAIN_LOG" | head -n1 | grep -oE '[0-9]+')
    #peak_mem=$(tail -n1 "$MAIN_LOG" | grep -oE '[0-9]+')
    #echo $num_subnets,$num_hosts,$DOP,$num_nodes,$num_links,all,N/A,N/A,$total_time,$peak_mem
done
