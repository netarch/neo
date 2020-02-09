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

echo '# of LBs, # of servers, algorithm, # of concurrent processes, # of repetition, # of nodes, # of links, Policy, # of ECs, # of communications, Time (microseconds), Memory (kilobytes)'

for dir in "$OUT_DIR"/*; do
    [ ! -d "$dir" -o "$dir" == *".latency" ] && continue

    num_lbs=$(basename "$dir" | cut -d '.' -f 1 | sed 's/-lbs//')
    num_servers=$(basename "$dir" | cut -d '.' -f 2 | sed 's/-servers//')
    algo=$(basename "$dir" | cut -d '.' -f 3)
    repetition=$(basename "$dir" | cut -d '.' -f 4 | sed 's/repeat-//')
    DOP=$(basename "$dir" | cut -d '.' -f 5 | sed 's/DOP-//')

    for policy in "$dir"/*; do
        [ ! -d "$policy" ] && continue

        # output per-policy results
        if [ "$algo" == 'sh' ]; then
            repetition="$(grep 'LBPolicy: repeated' "$policy"/*.log | head -n1 | grep -oE '[0-9]+')"
        fi
        policy_result="$(tail -n1 "$policy/stats.csv")"
        echo $num_lbs,$num_servers,$algo,$DOP,$repetition,$policy_result
    done

    # output per-network-setting results
    MAIN_LOG="$dir"/main.log
    num_nodes=$(head -n2 "$MAIN_LOG" | grep -E 'Loaded [0-9]+ nodes' | grep -oE '[0-9]+')
    num_links=$(head -n2 "$MAIN_LOG" | grep -E 'Loaded [0-9]+ links' | grep -oE '[0-9]+')
    total_time=$(tail -n2 "$MAIN_LOG" | head -n1 | grep -oE '[0-9]+')
    peak_mem=$(tail -n1 "$MAIN_LOG" | grep -oE '[0-9]+')
    echo $num_lbs,$num_servers,$algo,$DOP,$repetition,$num_nodes,$num_links,all,N/A,N/A,$total_time,$peak_mem
done
