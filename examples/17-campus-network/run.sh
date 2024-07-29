#!/bin/bash

SCRIPT_DIR="$(dirname "$(realpath "${BASH_SOURCE[0]}")")"
source "${SCRIPT_DIR}/../common.sh"

# # Test
# emu_pct=8
# invs=4
# "${CONFGEN[@]}" -e "$emu_pct" -i "$invs" >"$CONF"
# procs=4
# drop=timeout
# name="output.$emu_pct-emulated.$invs-invariants.$procs-procs.$drop"
# run "$name" "$procs" "$drop" "$CONF" #--parallel-invs
# rm "$CONF"

for emu_pct in 4 8 12 16 20; do
    for invs in 1 4 8 12 16; do
        "${CONFGEN[@]}" -e "$emu_pct" -i "$invs" >"$CONF"
        for procs in 1 4 8 12 16; do
            for drop in "${DROP_METHODS[@]}"; do
                name="output.$emu_pct-emulated.$invs-invariants.$procs-procs.$drop"
                run "$name" "$procs" "$drop" "$CONF"
            done
        done
        rm "$CONF"
    done
done

msg "Done"
