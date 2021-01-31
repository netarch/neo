#!/bin/bash

SCRIPT_DIR="$(dirname $(realpath ${BASH_SOURCE[0]}))"
source "${SCRIPT_DIR}/../common.sh"

build

for lbs in 4 8 12; do
    for srvs in 4 8 12; do
        for algo in rr sh dh; do    # round-robin, source-hashing, destination-hashing
            if [ "$algo" == "sh" ]; then
                repeat=$((srvs * 4))
            else
                repeat=$srvs
            fi
            for procs in 1; do
                name="$lbs-lbs.$srvs-servers.$algo.repeat-$repeat.DOP-$procs"
                msg "Verifying $name"
                ${CONFGEN[*]} -l $lbs -s $srvs -a $algo -r $repeat > "$CONF"
                sudo "$NEO" -fj $procs -i "$CONF" -o "$RESULTS_DIR/$name"
                sudo chown -R $(id -u):$(id -g) "$RESULTS_DIR/$name"
                mv "$CONF" "$RESULTS_DIR/$name"
            done
        done
    done
done

msg "Done"
