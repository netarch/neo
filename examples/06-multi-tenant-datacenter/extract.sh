#!/bin/bash

SCRIPT_DIR="$(dirname $(realpath ${BASH_SOURCE[0]}))"
source "${SCRIPT_DIR}/../common.sh"

echo '# of tenants, # of updates, # of concurrent processes, # of nodes, # of links, Policy, # of ECs, # of communications, Time (microseconds), Memory (kilobytes)'

for dir in "$RESULTS_DIR"/*; do
    [ ! -d "$dir" -o "$dir" == *".latency" ] && continue

    num_tenants=$(basename "$dir" | cut -d '.' -f 1 | sed 's/-tenants//')
    num_updates=$(basename "$dir" | cut -d '.' -f 2 | sed 's/-updates//')
    DOP=$(basename "$dir" | cut -d '.' -f 3 | sed 's/DOP-//')

    for policy in "$dir"/*; do
        [ ! -d "$policy" ] && continue

        # output per-policy results
        policy_result="$(tail -n1 "$policy/policy.stats.csv")"
        echo $num_tenants,$num_updates,$DOP,$policy_result
    done

    # output per-network-setting results
    MAIN_LOG="$dir"/main.log
    num_nodes=$(head -n3 "$MAIN_LOG" | grep -oE 'Loaded [0-9]+ nodes' | grep -oE '[0-9]+')
    num_links=$(head -n3 "$MAIN_LOG" | grep -oE 'Loaded [0-9]+ links' | grep -oE '[0-9]+')
    total_time=$(tail -n2 "$MAIN_LOG" | head -n1 | grep -oE 'Time: .*$' | grep -oE '[0-9]+')
    peak_mem=$(tail -n1 "$MAIN_LOG" | grep -oE 'Memory: .*$' | grep -oE '[0-9]+')
    echo $num_tenants,$num_updates,$DOP,$num_nodes,$num_links,all,N/A,N/A,$total_time,$peak_mem
done
