#!/bin/bash

SCRIPT_DIR="$(dirname $(realpath ${BASH_SOURCE[0]}))"
source "${SCRIPT_DIR}/../common.sh"

build

for subnets in 5 10 15; do
    for hosts in 5 10 15; do
        for procs in 1 8 16 24; do
            name="$subnets-subnets.$hosts-hosts.DOP-$procs"
            msg "Verifying $name"
            ${CONFGEN[*]} --subnets $subnets --hosts $hosts > "$CONF"
            sudo "$NEO" -fj $procs -i "$CONF" -o "$RESULTS_DIR/$name"
            sudo chown -R $(id -u):$(id -g) "$RESULTS_DIR/$name"
            mv "$CONF" "$RESULTS_DIR/$name"

            name="$subnets-subnets.$hosts-hosts.DOP-$procs.fault"
            msg "Verifying $name"
            ${CONFGEN[*]} --subnets $subnets --hosts $hosts --fault > "$CONF"
            sudo "$NEO" -fj $procs -i "$CONF" -o "$RESULTS_DIR/$name"
            sudo chown -R $(id -u):$(id -g) "$RESULTS_DIR/$name"
            mv "$CONF" "$RESULTS_DIR/$name"
        done
    done
done

msg "Done"
