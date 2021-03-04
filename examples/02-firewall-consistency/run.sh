#!/bin/bash

SCRIPT_DIR="$(dirname $(realpath ${BASH_SOURCE[0]}))"
source "${SCRIPT_DIR}/../common.sh"

build

for apps in 4 8 12; do
    hosts=$apps
    ${CONFGEN[*]} --apps $apps --hosts $hosts > "$CONF"
    for procs in 1 4 8 16; do
        name="$apps-apps.$hosts-hosts.DOP-$procs"
        msg "Verifying $name"
        sudo "$NEO" -fj $procs -i "$CONF" -o "$RESULTS_DIR/$name"
        sudo chown -R $(id -u):$(id -g) "$RESULTS_DIR/$name"
        cp "$CONF" "$RESULTS_DIR/$name"
    done
done

## faulty configuration
#for apps in 4 8 12; do
#    hosts=$apps
#    ${CONFGEN[*]} --apps $apps --hosts $hosts --fault > "$CONF"
#    for procs in 1 4 8 16; do
#        name="$apps-apps.$hosts-hosts.DOP-$procs.fault"
#        msg "Verifying $name"
#        sudo "$NEO" -fj $procs -i "$CONF" -o "$RESULTS_DIR/$name"
#        sudo chown -R $(id -u):$(id -g) "$RESULTS_DIR/$name"
#        cp "$CONF" "$RESULTS_DIR/$name"
#    done
#done

msg "Done"
