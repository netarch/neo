#!/bin/bash

SCRIPT_DIR="$(dirname $(realpath ${BASH_SOURCE[0]}))"
source "${SCRIPT_DIR}/../common.sh"

echo '# of applications, # of hosts, # of concurrent processes, # of nodes, # of links, Policy, # of ECs, # of communications, Time (microseconds), Memory (kilobytes)'

for dir in "$RESULTS_DIR"/*; do
    [ ! -d "$dir" -o "$dir" == *".latency" ] && continue

    num_apps=$(basename "$dir" | cut -d '.' -f 1 | sed 's/-apps//')
    num_hosts=$(basename "$dir" | cut -d '.' -f 2 | sed 's/-hosts//')
    DOP=$(basename "$dir" | cut -d '.' -f 3 | sed 's/DOP-//')

    for policy in "$dir"/*; do
        [ ! -d "$policy" ] && continue

        #policy_time=0
        #policy_mem=0
        #for res in "$RESULTS_DIR"; do
        #    time=$(tail -n1 "$res/$(basename "$dir")/$(basename "$policy")/policy.stats.csv" | cut -d ',' -f 6)
        #    mem=$(tail -n1 "$res/$(basename "$dir")/$(basename "$policy")/policy.stats.csv" | cut -d ',' -f 7)
        #    ((policy_time+=$time))
        #    ((policy_mem+=$mem))
        #done
        #policy_time=$(bc -l <<< "$policy_time/$#")
        #policy_mem=$(bc -l <<< "$policy_mem/$#")

        # output per-policy results
        #policy_result="$(tail -n1 "$policy/policy.stats.csv" | awk -F ',' '{print $1 "," $2 "," $3 "," $4 "," $5}')"
        #echo $num_apps,$num_hosts,$DOP,$policy_result,$policy_time,$policy_mem
        policy_result="$(tail -n1 "$policy/policy.stats.csv")"
        echo $num_apps,$num_hosts,$DOP,$policy_result
    done

    # output per-network-setting results
    #total_time=0
    #peak_mem=0
    #for res in "$RESULTS_DIR"; do
    #    main_log="$res/$(basename "$dir")/main.log"
    #    ((total_time+=$(tail -n2 "$main_log" | head -n1 | grep -oE 'Time: .*$' | grep -oE '[0-9]+')))
    #    ((peak_mem+=$(tail -n1 "$main_log" | grep -oE 'Memory: .*$' | grep -oE '[0-9]+')))
    #done
    #total_time=$(bc -l <<< "$total_time/$#")
    #peak_mem=$(bc -l <<< "$peak_mem/$#")

    MAIN_LOG="$dir"/main.log
    num_nodes=$(head -n3 "$MAIN_LOG" | grep -oE 'Loaded [0-9]+ nodes' | grep -oE '[0-9]+')
    num_links=$(head -n3 "$MAIN_LOG" | grep -oE 'Loaded [0-9]+ links' | grep -oE '[0-9]+')
    total_time=$(tail -n2 "$MAIN_LOG" | head -n1 | grep -oE 'Time: .*$' | grep -oE '[0-9]+')
    peak_mem=$(tail -n1 "$MAIN_LOG" | grep -oE 'Memory: .*$' | grep -oE '[0-9]+')
    echo $num_apps,$num_hosts,$DOP,$num_nodes,$num_links,all,N/A,N/A,$total_time,$peak_mem
done
