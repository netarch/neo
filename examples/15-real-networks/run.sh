#!/bin/bash

SCRIPT_DIR="$(dirname "$(realpath "${BASH_SOURCE[0]}")")"
source "${SCRIPT_DIR}/../common.sh"

networks=(
    as-733.103-nodes.239-edges.txt
    as-733.1470-nodes.3131-edges.txt
    # oregon-1.10670-nodes.22002-edges.txt
    rocketfuel-bb-AS-1221.108-nodes.153-edges.txt
    rocketfuel-bb-AS-1239.315-nodes.972-edges.txt
    rocketfuel-bb-AS-1755.87-nodes.161-edges.txt
    rocketfuel-bb-AS-3257.161-nodes.328-edges.txt
    rocketfuel-bb-AS-3967.79-nodes.147-edges.txt
    rocketfuel-bb-AS-6461.141-nodes.374-edges.txt
    # rocketfuel-cch-AS-1221.3497-nodes.4010-edges.txt
    # rocketfuel-cch-AS-1239.9903-nodes.12520-edges.txt
    # rocketfuel-cch-AS-1755.300-nodes.548-edges.txt
    # rocketfuel-cch-AS-2914.5939-nodes.8902-edges.txt
    # rocketfuel-cch-AS-3257.472-nodes.716-edges.txt
    # rocketfuel-cch-AS-3356.1772-nodes.6897-edges.txt
    # rocketfuel-cch-AS-3967.443-nodes.917-edges.txt
    # rocketfuel-cch-AS-4755.142-nodes.190-edges.txt
    # rocketfuel-cch-AS-6461.646-nodes.1323-edges.txt
    # rocketfuel-cch-AS-7018.11292-nodes.13700-edges.txt
)

for network in "${networks[@]}"; do
    network_name="${network%%.*}"
    for emu_pct in 4 8 12 16; do
        for inv_eps in 2 3 4; do
            "${CONFGEN[@]}" --topo "$network" -e "$emu_pct" -i "$inv_eps" >"$CONF"
            for procs in 1 4 8 12 16; do
                for drop in "${DROP_METHODS[@]}"; do
                    name="output.$network_name.$emu_pct-emulated.$inv_eps-endpoints.$procs-procs.$drop"
                    run "$name" "$procs" "$drop" "$CONF"
                done
            done
            rm "$CONF"
        done
    done
done
