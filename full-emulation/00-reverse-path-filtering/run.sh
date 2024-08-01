#!/bin/bash

SCRIPT_DIR="$(dirname "$(realpath "${BASH_SOURCE[0]}")")"
source "$SCRIPT_DIR/../common.sh"
cd "$SCRIPT_DIR"

# Prepare inputs: CONF, BRIDGES_TXT, DEVICES_TXT
toml="$SCRIPT_DIR/../../examples/00-reverse-path-filtering/network.fault.toml"
if [[ ! -e "$CONF" ]]; then
    "${CONFGEN[@]}" --network "$toml" >"$CONF"
fi
"${CONFGEN[@]}" --network "$toml" --bridges >"$BRIDGES_TXT"
"${CONFGEN[@]}" --network "$toml" --devices >"$DEVICES_TXT"

for i in {1..100}; do
    msg "Test $i ..."
    /usr/bin/time bash "$SCRIPT_DIR/single-run.sh" &>"$RESULTS_DIR/$i.log"
done

msg "Done"
