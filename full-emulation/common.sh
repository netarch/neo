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
if [ -z "${TOML_INPUT+x}" ]; then
    die '"TOML_INPUT" is unset'
fi
if [[ ! -e "$TOML_INPUT" ]]; then
    die "File not found: $TOML_INPUT"
fi

PROJECT_DIR="$(realpath "$(dirname "${BASH_SOURCE[0]}")"/..)"
CONF="$SCRIPT_DIR/network.clab.yml"
CONFGEN=("python3" "$SCRIPT_DIR/confgen.py")
BRIDGES_TXT="$(realpath "$SCRIPT_DIR/bridges.txt")"
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

cleanup() {
    set +e
    sudo containerlab destroy -t "$CONF"
    delete_bridges

    unfinished_cntrs="$(docker ps -a -q)"
    if [[ -n "$unfinished_cntrs" ]]; then
        make -C "$PROJECT_DIR/Dockerfiles" clean
    fi

    # rm -rf clab-* .*.bak
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

prepull() {
    local images=(
        kyechou/ipvs
        kyechou/iptables
        kyechou/linux-router
        kyechou/klint-faulty-nat
        kyechou/klint-nat
        kyechou/klint-maglev
        kyechou/klint-firewall
        kyechou/click
        kyechou/dpdk
    )
    for image in "${images[@]}"; do
        docker pull "$image:latest"
    done
}

_main() {
    prepull

    # Prepare input for containerlab
    if [[ ! -e "$CONF" ]]; then
        "${CONFGEN[@]}" --network "$TOML_INPUT" >"$CONF"
    fi
    "${CONFGEN[@]}" --network "$TOML_INPUT" --bridges >"$BRIDGES_TXT"

    mkdir -p "$RESULTS_DIR"
    trap int_handler SIGINT
}

_main "$@"
