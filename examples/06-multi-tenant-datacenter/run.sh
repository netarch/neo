#!/bin/bash

SCRIPT_DIR="$(dirname "$(realpath "${BASH_SOURCE[0]}")")"
source "${SCRIPT_DIR}/../common.sh"

for tenants in 4 8 16 32 64; do
    for updates in 0 $((tenants / 2)) $tenants; do
        "${CONFGEN[@]}" -t "$tenants" -u "$updates" -s 1 >"$CONF"
        for procs in 1 4 8 12 16; do
            for drop in "${DROP_METHODS[@]}"; do
                name="output.$tenants-tenants.$updates-updates.$procs-procs.$drop"
                run "$name" "$procs" "$drop" "$CONF"
            done
        done
        rm "$CONF"
    done
done

msg "Done"
