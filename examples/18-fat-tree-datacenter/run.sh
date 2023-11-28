#!/bin/bash

SCRIPT_DIR="$(dirname "$(realpath "${BASH_SOURCE[0]}")")"
source "${SCRIPT_DIR}/../common.sh"

for arity in 4 6 8 10 12; do
    "${CONFGEN[@]}" -k "$arity" -s 1 >"$CONF"
    for procs in 1 4 8 12 16; do
        for drop in "${DROP_METHODS[@]}"; do
            name="output.$arity-ary.$procs-procs.$drop"
            run "$name" "$procs" "$drop" "$CONF"
        done
    done
    rm "$CONF"
done

msg "Done"
