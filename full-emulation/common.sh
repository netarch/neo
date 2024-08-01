#!/bin/bash

set -euo pipefail

msg() {
    echo -e "[+] ${1-}" >&2
}

hurt() {
    echo -e "[-] ${1-}" >&2
}

warn() {
    echo -e "[!] ${1-}" >&2
}

die() {
    echo -e "[!] ${1-}" >&2
    exit 1
}

if [ $UID -eq 0 ]; then
    die 'Please run this script without root privilege'
fi

if [ -z "${SCRIPT_DIR+x}" ]; then
    die '"SCRIPT_DIR" is unset'
fi

PROJECT_DIR="$(realpath "$(dirname "${BASH_SOURCE[0]}")"/..)"
CONF="$SCRIPT_DIR/network.clab.yml"
CONFGEN=("python3" "$SCRIPT_DIR/confgen.py")
BRIDGES_TXT="$(realpath "$SCRIPT_DIR/bridges.txt")"
DEVICES_TXT="$(realpath "$SCRIPT_DIR/devices.txt")"
RESULTS_DIR="$(realpath "$SCRIPT_DIR/results")"
export CONF
export CONFGEN

create_bridges() {
    mapfile -t bridges <"$BRIDGES_TXT"
    for br in "${bridges[@]}"; do
        sudo ip link add name "$br" type bridge
        sudo ip link set dev "$br" up
    done
}

delete_bridges() {
    mapfile -t bridges <"$BRIDGES_TXT"
    for br in "${bridges[@]}"; do
        sudo ip link set dev "$br" down
        sudo ip link delete "$br" type bridge
    done
}

startup() {
    create_bridges
    sudo containerlab deploy -t "$CONF"
}

get_container_memories() {
    mapfile -t devices <"$DEVICES_TXT"
    total_mem_kb=0
    for device in "${devices[@]}"; do
        pid="$(docker inspect -f '{{.State.Pid}}' "$device")"
        mem_kb="$(grep VmPeak "/proc/$pid/status" | awk -F ' ' '{print $2}')"
        total_mem_kb=$((total_mem_kb + mem_kb))
    done
    msg "Total container memory: $total_mem_kb KB"
}

cleanup() {
    set +e
    sudo containerlab destroy -t "$CONF"
    delete_bridges

    unfinished_cntrs="$(docker ps -a -q)"
    if [[ -n "$unfinished_cntrs" ]]; then
        make -C "$PROJECT_DIR/Dockerfiles" clean
    fi

    rm -rf "$SCRIPT_DIR"/clab-* "$SCRIPT_DIR"/.*.bak
    set -e
}

# run() {
#     name="$1"
#     procs="$2"
#     drop="$3"
#     infile="$4"
#     outdir="$RESULTS_DIR/$name"
#     outlog="$SCRIPT_DIR/out.log"
#     shift 4
#     args=("$@")
#     msg "Verifying $name"
#     sudo /usr/bin/time "$NEO" -f -j "$procs" -d "$drop" -i "$infile" -o "$outdir" "${args[@]}" \
#         2>&1 | tee "$outlog" >/dev/null
#     cleanup
#     sudo chown -R "$(id -u):$(id -g)" "$outdir"
#     mv "$outlog" "$outdir/"
#     cp "$infile" "$outdir/network.toml"
# }

int_handler() {
    set +e
    hurt "Interrupted. Closing the running processes and containers..."
    cleanup
    # sudo chown -R "$(id -u):$(id -g)" "$outdir"
    # cp "$infile" "$outdir/network.toml"
    hurt "Exit (interrupted)"
    exit 1
}

_main() {
    mkdir -p "$RESULTS_DIR"
    trap int_handler SIGINT
}

_main "$@"
