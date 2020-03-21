#!/bin/bash

set -e
set -o nounset

[ $UID -eq 0 ] && \
    (echo '[!] Please run this script without root privilege' >&2; exit 1)

[ $# -eq 0 ] && \
    (echo "[!] Usage: $0 <dir>" >&2; exit 1)

OUT_DIR="$(realpath "$*")"

[ ! -d "$OUT_DIR" ] && \
    (echo "[!] Directory not found: $OUT_DIR" >&2; exit 1)

for dir in "$OUT_DIR"/*.latency; do
    [ ! -d "$dir" ] && continue

    experiment=$(basename "$dir")

    for policy in "$dir"/*; do
        [ ! -d "$policy" ] && continue

        echo "$experiment/$(basename "$policy")"

        first=1
        for log in "$policy"/*.stats.csv; do
            if [ $first -eq 1 ]; then
                first=0
                head -n3 $log | tail -n1
            fi
            tail -n+4 $log
        done
    done
done
