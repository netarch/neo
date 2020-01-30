#!/bin/bash

set -e
set -o nounset

SCRIPT_DIR="$(dirname $(realpath ${BASH_SOURCE[0]}))"
cd "$SCRIPT_DIR"

[ $UID -eq 0 ] && \
    (echo '[!] Please run this script without root privilege' >&2; exit 1)

NEO="$(realpath "$SCRIPT_DIR/../../neo")"
OUT_DIR="$(realpath "$SCRIPT_DIR/results")"

type "$NEO" >/dev/null 2>&1 || \
    (echo '[!] Verifier executable not found' >&2; exit 1)

mkdir -p "$OUT_DIR"

for num_subnets in 5 10 15; do
    for num_hosts in 5 10 15; do
        for num_procs in 1; do
            echo "[+] Verifying $num_subnets subnets/zone and $num_hosts hosts/subnet..."
            python confgen.py --subnets $num_subnets --hosts $num_hosts \
                > network.toml
            sudo "$NEO" -f -j $num_procs -i network.toml \
                -o "$OUT_DIR/$num_subnets-subnets.$num_hosts-hosts.DOP-$num_procs"
            sudo "$NEO" -l -f -j $num_procs -i network.toml \
                -o "$OUT_DIR/$num_subnets-subnets.$num_hosts-hosts.DOP-$num_procs.latency"
        done
    done
done

echo "[+] Done!"
