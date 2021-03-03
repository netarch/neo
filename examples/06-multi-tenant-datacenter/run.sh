#!/bin/bash

SCRIPT_DIR="$(dirname $(realpath ${BASH_SOURCE[0]}))"
source "${SCRIPT_DIR}/../common.sh"

build

for tenants in 5 10 15; do
    for updates in 0 $((tenants / 2)) $tenants; do
        for servers in 1; do
            for procs in 1 8 16 24; do
                name="$tenants-tenants.$updates-updates.$servers-servers.DOP-$procs"
                msg "Verifying $name"
                ${CONFGEN[*]} -t $tenants -u $updates -s $servers > "$CONF"
                sudo "$NEO" -fj $procs -i "$CONF" -o "$RESULTS_DIR/$name"
                sudo chown -R $(id -u):$(id -g) "$RESULTS_DIR/$name"
                mv "$CONF" "$RESULTS_DIR/$name"
            done
        done
    done
done

msg "Done"
