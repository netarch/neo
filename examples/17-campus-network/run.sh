#!/bin/bash

SCRIPT_DIR="$(dirname "$(realpath "${BASH_SOURCE[0]}")")"
source "${SCRIPT_DIR}/../common.sh"

networks=(
    core1
    core2
    core4
    core5
    core6
    core8
    core9
    core10
)

# # Test
# network=core1
# "${CONFGEN[@]}" -n "$network" >"$CONF"
# procs=1
# drop=timeout
# name="output.network-$network.$procs-procs.$drop"
# run "$name" "$procs" "$drop" "$CONF" #--parallel-invs
# rm "$CONF"

for network in "${networks[@]}"; do
    "${CONFGEN[@]}" -n "$network" >"$CONF"
    procs=1
    drop=timeout
    name="output.network-$network.$procs-procs.$drop"
    run "$name" "$procs" "$drop" "$CONF"
    rm "$CONF"
done

msg "Done"
