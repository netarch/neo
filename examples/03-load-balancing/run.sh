#!/bin/bash

SCRIPT_DIR="$(dirname "$(realpath "${BASH_SOURCE[0]}")")"
source "${SCRIPT_DIR}/../common.sh"

for lbs in 2 4; do
    srvs=$lbs
    for algo in rr sh dh; do # round-robin, source-hashing, destination-hashing
        "${CONFGEN[@]}" -l $lbs -s $srvs -a $algo > "$CONF"
        for procs in 1 4 8 16; do
            for drop in "${DROP_METHODS[@]}"; do
                name="output.$lbs-lbs.$srvs-servers.algo-$algo.$procs-procs.$drop"
                run "$name" "$procs" "$drop" "$CONF"
            done
        done
        rm "$CONF"
    done
done

msg "Done"
