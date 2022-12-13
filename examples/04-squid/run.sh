#!/bin/bash

SCRIPT_DIR="$(dirname "$(realpath "${BASH_SOURCE[0]}")")"
source "${SCRIPT_DIR}/../common.sh"

sudo "$NEO" -fj16 -i "$CONF" -o "$RESULTS_DIR"
sudo chown -R "$(id -u):$(id -g)" "$RESULTS_DIR"
cp "$CONF" "$RESULTS_DIR"
rm "$CONF"

# msg "Populating measurements..."
# extract_stats "$RESULTS_DIR" > "$RESULTS_DIR/stats.csv"
# extract_latency "$RESULTS_DIR" > "$RESULTS_DIR/latency.csv"

# msg "Collecting false violations..."
# true > "$RESULTS_DIR/false_violations.txt"
# for dir in "$RESULTS_DIR"/*; do
#     [[ ! -d "$dir" || "$dir" == *"fault"* ]] && continue
#     if grep violated "$dir/main.log" >/dev/null; then
#         basename "$dir" >> "$RESULTS_DIR/false_violations.txt"
#     fi
# done

msg "Done"
