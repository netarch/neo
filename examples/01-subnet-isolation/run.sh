#!/bin/bash

SCRIPT_DIR="$(dirname $(realpath ${BASH_SOURCE[0]}))"
source "${SCRIPT_DIR}/../common.sh"

build

for num_subnets in 5 10 15; do
    for num_hosts in 5 10 15; do
        for num_procs in 1; do
            msg "Verifying $num_subnets subnets/zone and $num_hosts hosts/subnet..."
            ${CONFGEN[*]} --subnets $num_subnets --hosts $num_hosts > "$CONF"
            sudo "$NEO" -fj $num_procs -i "$CONF" \
                -o "$RESULTS_DIR/$num_subnets-subnets.$num_hosts-hosts.DOP-$num_procs"
            #sudo "$NEO" -fj $num_procs -i "$CONF" -l \
            #    -o "$RESULTS_DIR/$num_subnets-subnets.$num_hosts-hosts.DOP-$num_procs.latency"


#            msg "Verifying $num_subnets subnets/zone and $num_hosts hosts/subnet..."
#            ${CONFGEN[*]} --subnets $num_subnets --hosts $num_hosts --fault1 \
#                > network.fault1.toml
#            ${CONFGEN[*]} --subnets $num_subnets --hosts $num_hosts --fault2 \
#                > network.fault2.toml
#            sudo "$NEO" -f -j $num_procs -i network.fault1.toml \
#                -o "$RESULTS_DIR/$num_subnets-subnets.$num_hosts-hosts.DOP-$num_procs.fault1"
#            sudo "$NEO" -f -j $num_procs -i network.fault2.toml \
#                -o "$RESULTS_DIR/$num_subnets-subnets.$num_hosts-hosts.DOP-$num_procs.fault2"
#
#            sudo rm -rf "$RESULTS_DIR/$num_subnets-subnets.$num_hosts-hosts.DOP-$num_procs.fault1/4"
#            sudo mv "$RESULTS_DIR/$num_subnets-subnets.$num_hosts-hosts.DOP-$num_procs.fault2/1" "$RESULTS_DIR/$num_subnets-subnets.$num_hosts-hosts.DOP-$num_procs.fault1/4"
#            sudo rm -rf "$RESULTS_DIR/$num_subnets-subnets.$num_hosts-hosts.DOP-$num_procs.fault2"
#            sudo sed -i "$RESULTS_DIR/$num_subnets-subnets.$num_hosts-hosts.DOP-$num_procs.fault1/4/stats.csv" -e '$s/^\([0-9]\+,[ ]*[0-9]\+,\)[ ]*1/\1 4/'
        done
    done
done

msg "Done"
