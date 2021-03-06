#!/bin/bash

SCRIPT_DIR="$(dirname $(realpath ${BASH_SOURCE[0]}))"
source "${SCRIPT_DIR}/../common.sh"

build -DVECTORSZ=3000 -DMAX_CONNS=30

for lbs in 2 4; do
    srvs=$lbs
    for algo in rr sh dh; do    # round-robin, source-hashing, destination-hashing
        ${CONFGEN[*]} -l $lbs -s $srvs -a $algo > "$CONF"
        for procs in 1 4 8 16; do
            # timeout
            name="$lbs-lbs.$srvs-servers.algo-$algo.DOP-$procs"
            msg "Verifying $name"
            sudo "$NEO" -fj $procs -i "$CONF" -o "$RESULTS_DIR/$name"
            sudo chown -R $(id -u):$(id -g) "$RESULTS_DIR/$name"
            cp "$CONF" "$RESULTS_DIR/$name"
            # dropmon
            name="$lbs-lbs.$srvs-servers.algo-$algo.DOP-$procs.dropmon"
            msg "Verifying $name"
            sudo "$NEO" -fdj $procs -i "$CONF" -o "$RESULTS_DIR/$name"
            sudo chown -R $(id -u):$(id -g) "$RESULTS_DIR/$name"
            cp "$CONF" "$RESULTS_DIR/$name"
        done
    done
done

msg "Populating measurements..."

extract_stats() {
    results_dir="$1"
    [[ ! -d "$results_dir" ]] && die "Invalid directory: $results_dir"

    echo 'algorithm, fault, dropmon, parallel processes, nodes, links, Policy, Connection ECs, Time (microseconds), Memory (kilobytes)'
    for dir in "$results_dir"/*; do
        [[ ! -d "$dir" ]] && continue
        name="$(basename "$dir")"
        oldIFS=$IFS
        IFS="."
        for parameter in $name; do
            if [[ "$parameter" == "algo-"* ]]; then
                algorithm=$(echo $parameter | sed 's/algo-//')
            fi
        done
        IFS=$oldIFS
        echo "$algorithm, $(extract_per_session_stats $dir)"
    done
}
extract_stats "$RESULTS_DIR" > "$RESULTS_DIR/stats.csv"

extract_latency() {
    results_dir="$1"
    [[ ! -d "$results_dir" ]] && die "Invalid directory: $results_dir"

    echo -n 'algorithm, fault, dropmon, parallel processes, nodes, links, Policy, Connection ECs, '
    echo -n 'Overall latency t1 (nsec), Overall latency (nsec), '
    echo -n 'Rewind latency t1 (nsec), Rewind latency (nsec), Rewind injection count, '
    echo -n 'Packet latency t1 (nsec), Packet latency (nsec), Timeout (nsec), '
    echo    'Kernel drop latency (nsec)'
    for dir in "$results_dir"/*; do
        [[ ! -d "$dir" ]] && continue
        name="$(basename "$dir")"
        oldIFS=$IFS
        IFS="."
        for parameter in $name; do
            if [[ "$parameter" == "algo-"* ]]; then
                algorithm=$(echo $parameter | sed 's/algo-//')
            fi
        done
        IFS=$oldIFS
        echo "$algorithm, $(extract_per_session_latency $dir)"
    done
}
extract_latency "$RESULTS_DIR" > "$RESULTS_DIR/latency.csv"

msg "Collecting false violations..."
> "$RESULTS_DIR/false_violations.txt"
for dir in "$RESULTS_DIR"/*; do
    [[ ! -d "$dir" || "$dir" == *"algo-dh"* ]] && continue
    if grep violated "$dir/main.log" >/dev/null; then
        echo "$(basename $dir)" >> "$RESULTS_DIR/false_violations.txt"
    fi
done

msg "Done"
