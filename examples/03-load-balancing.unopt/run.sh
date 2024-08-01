#!/bin/bash

SCRIPT_DIR="$(dirname "$(realpath "${BASH_SOURCE[0]}")")"
source "${SCRIPT_DIR}/../common.sh"

# This is necessary because the Docker sysctl configuration will happen before
# ipvsadm is invoked, when `ip_vs` hasn't been loaded yet.
if ! lsmod | grep ip_vs >/dev/null; then
    sudo modprobe ip_vs
fi

for lbs in 2 4 6; do
    srvs=$lbs
    # IPVS scheduling algorithms
    # https://keepalived-pqa.readthedocs.io/en/latest/scheduling_algorithms.html
    # source-hashing, destination-hashing, maglev-hashing, least-connection
    for algo in sh dh mh lc; do
        "${CONFGEN[@]}" -l $lbs -s $srvs -a $algo >"$CONF"
        procs=1
        drop=timeout
        name="output.$lbs-lbs.$srvs-servers.algo-$algo.$procs-procs.$drop"
        set +e
        timeout --signal=INT 1h bash "$SCRIPT_DIR/single-run.sh" "$name" "$procs" "$drop" "$CONF"
        set -e
        rm "$CONF"
    done
done

msg "Done"
