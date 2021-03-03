#!/bin/bash

SCRIPT_DIR="$(dirname $(realpath ${BASH_SOURCE[0]}))"
source "${SCRIPT_DIR}/../common.sh"

build

for apps in 4 8 12; do
    for hosts in 4 8 12; do
        for procs in 1 4 8 16; do
            name="$apps-apps.$hosts-hosts.DOP-$procs"
            msg "Verifying $name"
            ${CONFGEN[*]} --apps $apps --hosts $hosts > "$CONF"
            sudo "$NEO" -fj $procs -i "$CONF" -o "$RESULTS_DIR/$name"
            sudo chown -R $(id -u):$(id -g) "$RESULTS_DIR/$name"
            mv "$CONF" "$RESULTS_DIR/$name"

            name="$apps-apps.$hosts-hosts.DOP-$procs.fault"
            msg "Verifying $name"
            ${CONFGEN[*]} --apps $apps --hosts $hosts --fault > "$CONF"
            sudo "$NEO" -fj $procs -i "$CONF" -o "$RESULTS_DIR/$name"
            sudo chown -R $(id -u):$(id -g) "$RESULTS_DIR/$name"
            mv "$CONF" "$RESULTS_DIR/$name"
        done
    done
done

msg "Done"
