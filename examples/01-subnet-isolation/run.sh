#!/bin/bash

SCRIPT_DIR="$(dirname "$(realpath "${BASH_SOURCE[0]}")")"
source "${SCRIPT_DIR}/../common.sh"

for subnets in 4 8 10 12; do
    hosts=$subnets
    "${CONFGEN[@]}" --subnets $subnets --hosts $hosts > "$CONF"
    for procs in 1 4 8 16 20; do
        # timeout
        name="$subnets-subnets.$hosts-hosts.DOP-$procs"
        msg "Verifying $name"
        sudo "$NEO" -fj $procs -i "$CONF" -o "$RESULTS_DIR/$name"
        sudo chown -R "$(id -u):$(id -g)" "$RESULTS_DIR/$name"
        cp "$CONF" "$RESULTS_DIR/$name"
        # dropmon
        name="$subnets-subnets.$hosts-hosts.DOP-$procs.dropmon"
        msg "Verifying $name"
        sudo "$NEO" -fdj $procs -i "$CONF" -o "$RESULTS_DIR/$name"
        sudo chown -R "$(id -u):$(id -g)" "$RESULTS_DIR/$name"
        cp "$CONF" "$RESULTS_DIR/$name"
    done
    rm "$CONF"
done

# faulty configuration
for subnets in 4 8 10 12; do
    hosts=$subnets
    "${CONFGEN[@]}" --subnets $subnets --hosts $hosts --fault > "$CONF"
    for procs in 1 4 8 16 20; do
        # timeout
        name="$subnets-subnets.$hosts-hosts.DOP-$procs.fault"
        msg "Verifying $name"
        sudo "$NEO" -fj $procs -i "$CONF" -o "$RESULTS_DIR/$name"
        sudo chown -R "$(id -u):$(id -g)" "$RESULTS_DIR/$name"
        cp "$CONF" "$RESULTS_DIR/$name"
        # dropmon
        name="$subnets-subnets.$hosts-hosts.DOP-$procs.fault.dropmon"
        msg "Verifying $name"
        sudo "$NEO" -fdj $procs -i "$CONF" -o "$RESULTS_DIR/$name"
        sudo chown -R "$(id -u):$(id -g)" "$RESULTS_DIR/$name"
        cp "$CONF" "$RESULTS_DIR/$name"
    done
    rm "$CONF"
done

# msg "Populating measurements..."
# extract_stats "$RESULTS_DIR" > "$RESULTS_DIR/stats.csv"
# extract_latency "$RESULTS_DIR" > "$RESULTS_DIR/latency.csv"

# msg "Collecting false violations..."
# > "$RESULTS_DIR/false_violations.txt"
# for dir in "$RESULTS_DIR"/*; do
#     [[ ! -d "$dir" || "$dir" == *"fault"* ]] && continue
#     if grep violated "$dir/main.log" >/dev/null; then
#         echo "$(basename $dir)" >> "$RESULTS_DIR/false_violations.txt"
#     fi
# done

msg "Done"
