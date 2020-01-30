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

for num_apps in 4 8 12; do
    for num_hosts in 4 8 12; do
        for num_procs in 1; do
            echo "[+] Verifying $num_apps applications and $num_hosts hosts/app..."
            python confgen.py --apps $num_apps --hosts $num_hosts > network.toml
            sudo "$NEO" -f -j $num_procs -i network.toml \
                -o "$OUT_DIR/$num_apps-apps.$num_hosts-hosts.DOP-$num_procs"
        done
    done
done

echo "[+] Done!"
