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

for num_lbs in 4 8 12; do
    for num_srvs in 4 8 12; do
        for algo in rr sh dh; do
            for repeat in 0; do
                for num_procs in 1; do
                    echo "[+] Verifying $num_lbs LBs and $num_srvs servers/LB with algorithm $algo..."
                    python3 confgen.py -l $num_lbs -s $num_srvs -a $algo -r $repeat \
                        > network.toml
                    sudo "$NEO" -f -j $num_procs -i network.toml \
                        -o "$OUT_DIR/$num_lbs-lbs.$num_srvs-servers.$algo.repeat-$((repeat + num_srvs)).DOP-$num_procs"
                done
            done
        done
    done
done

echo "[+] Done!"
