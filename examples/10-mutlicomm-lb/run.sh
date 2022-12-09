#!/bin/bash

SCRIPT_DIR="$(dirname "$(realpath "${BASH_SOURCE[0]}")")"
source "${SCRIPT_DIR}/../common.sh"

lbs=2
srvs=4
algo=rr

"${CONFGEN[@]}" -l $lbs -s $srvs -a $algo > "$CONF"
for procs in 1 4 8 16; do
    name="$lbs-lbs.$srvs-servers.$algo.DOP-$procs"
    msg "Verifying $name"
    sudo "$NEO" -fj $procs -i "$CONF" -o "$RESULTS_DIR/$name"
    sudo chown -R "$(id -u):$(id -g)" "$RESULTS_DIR/$name"
    cp "$CONF" "$RESULTS_DIR/$name"
done

msg "Done"
