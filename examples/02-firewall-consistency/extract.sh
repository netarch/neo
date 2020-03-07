#!/bin/bash

set -e
set -o nounset

[ $UID -eq 0 ] && \
    (echo '[!] Please run this script without root privilege' >&2; exit 1)

[ $# -eq 0 ] && \
    (echo "[!] Usage: $0 <dir> [<dir> ...]" >&2; exit 1)

echo '# of applications, # of hosts, # of concurrent processes, # of nodes, # of links, Policy, # of ECs, # of communications, Time (microseconds), Memory (kilobytes)'

res_dir="$1"
[ ! -d "$res_dir" ] && \
    (echo "[!] Directory not found: $res_dir" >&2; exit 1)

for exp_dir in "$res_dir"/*; do
    [ ! -d "$exp_dir" -o "$exp_dir" == *".latency" ] && continue

    num_apps=$(basename "$exp_dir" | cut -d '.' -f 1 | sed 's/-apps//')
    num_hosts=$(basename "$exp_dir" | cut -d '.' -f 2 | sed 's/-hosts//')
    DOP=$(basename "$exp_dir" | cut -d '.' -f 3 | sed 's/DOP-//')

    for policy in "$exp_dir"/*; do
        [ ! -d "$policy" ] && continue

        policy_time=0
        policy_mem=0
        for res in "$@"; do
            time=$(tail -n1 "$res/$(basename "$exp_dir")/$(basename "$policy")/stats.csv" | cut -d ',' -f 6)
            mem=$(tail -n1 "$res/$(basename "$exp_dir")/$(basename "$policy")/stats.csv" | cut -d ',' -f 7)
            ((policy_time+=$time))
            ((policy_mem+=$mem))
        done
        policy_time=$(bc -l <<< "$policy_time/$#")
        policy_mem=$(bc -l <<< "$policy_mem/$#")

        # output per-policy results
        policy_result="$(tail -n1 "$policy/stats.csv" | awk -F ',' '{print $1 "," $2 "," $3 "," $4 "," $5}')"
        echo $num_apps,$num_hosts,$DOP,$policy_result,$policy_time,$policy_mem
    done

    # output per-network-setting results
    total_time=0
    peak_mem=0
    for res in "$@"; do
        main_log="$res/$(basename "$exp_dir")/main.log"
        ((total_time+=$(tail -n2 "$main_log" | head -n1 | grep -oE '[0-9]+')))
        ((peak_mem+=$(tail -n1 "$main_log" | grep -oE '[0-9]+')))
    done
    total_time=$(bc -l <<< "$total_time/$#")
    peak_mem=$(bc -l <<< "$peak_mem/$#")

    MAIN_LOG="$exp_dir"/main.log
    num_nodes=$(head -n2 "$MAIN_LOG" | grep -E 'Loaded [0-9]+ nodes' | grep -oE '[0-9]+')
    num_links=$(head -n2 "$MAIN_LOG" | grep -E 'Loaded [0-9]+ links' | grep -oE '[0-9]+')
    echo $num_apps,$num_hosts,$DOP,$num_nodes,$num_links,all,N/A,N/A,$total_time,$peak_mem
done
