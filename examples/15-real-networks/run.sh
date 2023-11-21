#!/bin/bash

SCRIPT_DIR="$(dirname "$(realpath "${BASH_SOURCE[0]}")")"
source "${SCRIPT_DIR}/../common.sh"
"${CONFGEN[@]}" --topology $1 --emulated $2 > "$CONF"

for procs in 1 4 8 16; do
  for drop in "${DROP_METHODS[@]}"; do
    name="output.$1.$2-emulated.$procs-procs.$drop"
    run "$name" "$procs" "$drop" "$CONF"
  done
done
rm "$CONF"
