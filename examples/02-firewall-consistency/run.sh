#!/bin/bash

SCRIPT_DIR="$(dirname "$(realpath "${BASH_SOURCE[0]}")")"
source "${SCRIPT_DIR}/../common.sh"

for apps in 4 8 12; do
    hosts=$apps
    "${CONFGEN[@]}" --apps $apps --hosts $hosts > "$CONF"
    for procs in 1 4 8 16; do
        for drop in "${DROP_METHODS[@]}"; do
            name="output.$apps-apps.$hosts-hosts.$procs-procs.$drop"
            run "$name" "$procs" "$drop" "$CONF"
        done
    done
    rm "$CONF"
done

# faulty configuration
for apps in 4 8 12; do
    hosts=$apps
    "${CONFGEN[@]}" --apps $apps --hosts $hosts --fault > "$CONF"
    for procs in 1 4 8 16; do
        for drop in "${DROP_METHODS[@]}"; do
            name="output.$apps-apps.$hosts-hosts.$procs-procs.$drop.fault"
            run "$name" "$procs" "$drop" "$CONF"
        done
    done
    rm "$CONF"
done

msg "Done"
