#!/bin/bash

SCRIPT_DIR="$(dirname "$(realpath "${BASH_SOURCE[0]}")")"
source "${SCRIPT_DIR}/../common.sh"

# # Test
# backends=8
# "${CONFGEN[@]}" --backends $backends >"$CONF"
# procs=1
# drop="timeout"
# name="output.$backends-backends.$procs-procs.$drop"
# run "$name" "$procs" "$drop" "$CONF"

for backends in 1 3 5 7 9; do
    "${CONFGEN[@]}" --backends $backends >"$CONF"
    for procs in 1 4 8 16; do
        drop="timeout"
        name="output.$backends-backends.$procs-procs.$drop"
        run "$name" "$procs" "$drop" "$CONF"
    done
    rm "$CONF"
done

msg "Done"
