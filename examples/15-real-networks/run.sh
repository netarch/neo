#!/bin/bash

SCRIPT_DIR="$(dirname "$(realpath "${BASH_SOURCE[0]}")")"
source "${SCRIPT_DIR}/../common.sh"

networks=(
    as-733.103-nodes.239-edges.txt
    as-733.1470-nodes.3131-edges.txt
    rocketfuel-bb-AS-1221.108-nodes.153-edges.txt
    rocketfuel-bb-AS-1239.315-nodes.972-edges.txt
    rocketfuel-bb-AS-1755.87-nodes.161-edges.txt
    rocketfuel-bb-AS-3257.161-nodes.328-edges.txt
    rocketfuel-bb-AS-3967.79-nodes.147-edges.txt
    rocketfuel-bb-AS-6461.141-nodes.374-edges.txt
)

# # Test
# for network in "${networks[@]}"; do
#     network_name="${network%.*}"
#     emu_pct=10
#     fw_pct=20
#     invs=10
#     "${CONFGEN[@]}" --topo "$network" -e "$emu_pct" -f "$fw_pct" -i "$invs" >"$CONF"
#     procs=8
#     drop=timeout
#     name="output.$network_name.$emu_pct-emulated.$fw_pct-fwleaves.$invs-invariants.$procs-procs.$drop"
#     run "$name" "$procs" "$drop" "$CONF" #--parallel-invs
#     rm "$CONF"
# done

for network in "${networks[@]}"; do
    network_name="${network%.*}"
    for emu_pct in 4 8 12 16 20; do
        fw_pct=20
        invs=16
        "${CONFGEN[@]}" --topo "$network" -e "$emu_pct" -f "$fw_pct" -i "$invs" >"$CONF"
        procs=1
        drop=timeout
        name="output.$network_name.$emu_pct-emulated.$fw_pct-fwleaves.$invs-invariants.$procs-procs.$drop"
        run "$name" "$procs" "$drop" "$CONF"
        rm "$CONF"
    done
done

msg "Done"
