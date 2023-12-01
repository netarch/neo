#!/bin/bash

SCRIPT_DIR="$(dirname "$(realpath "${BASH_SOURCE[0]}")")"
source "${SCRIPT_DIR}/../common.sh"

# # Test
# backends=4
# fw=iptables
# lb=klint
# "${CONFGEN[@]}" --backends $backends --firewall $fw --lb $lb >"$CONF"
# procs=1
# drop="timeout"
# name="output.$backends-backends.$fw-fw.$lb-lb.$procs-procs.$drop"
# run "$name" "$procs" "$drop" "$CONF"

backends=9
for fw in iptables klint; do
    for lb in ipvs klint; do
        "${CONFGEN[@]}" --backends $backends --firewall $fw --lb $lb >"$CONF"
        procs=1
        drop="timeout"
        name="output.$backends-backends.$fw-fw.$lb-lb.$procs-procs.$drop"
        run "$name" "$procs" "$drop" "$CONF"
        rm "$CONF"
    done
done

msg "Done"
