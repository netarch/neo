#!/bin/bash

SCRIPT_DIR="$(dirname "$(realpath "${BASH_SOURCE[0]}")")"
source "${SCRIPT_DIR}/../common.sh"

for arity in 4 6 8 10 12; do
    for update_pct in 0 50 100; do
        "${CONFGEN[@]}" -k "$arity" -u "$update_pct" -s 1 >"$CONF"
        for procs in 1 4 8 12 16; do
            for drop in "${DROP_METHODS[@]}"; do
                name="output.$arity-ary.$update_pct-update-pct.$procs-procs.$drop"
                run "$name" "$procs" "$drop" "$CONF"
            done
        done
        rm "$CONF"
    done
done

msg "Done"
