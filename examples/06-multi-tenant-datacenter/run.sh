#!/bin/bash

SCRIPT_DIR="$(dirname $(realpath ${BASH_SOURCE[0]}))"
source "${SCRIPT_DIR}/../common.sh"

build

for tenants in 4 8 12 32 64; do
    for updates in 0 $((tenants / 2)) $tenants; do
        ${CONFGEN[*]} -t $tenants -u $updates -s 1 > "$CONF"
        for procs in 1 4 8 16; do
            name="$tenants-tenants.$updates-updates.DOP-$procs"
            msg "Verifying $name"
            sudo "$NEO" -fj $procs -i "$CONF" -o "$RESULTS_DIR/$name"
            sudo chown -R $(id -u):$(id -g) "$RESULTS_DIR/$name"
            cp "$CONF" "$RESULTS_DIR/$name"
        done
    done
done

msg "Done"
