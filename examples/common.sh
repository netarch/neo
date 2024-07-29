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
NEO="$PROJECT_DIR/build/neo"
CONF="$SCRIPT_DIR/network.toml"
CONFGEN=("python3" "$SCRIPT_DIR/confgen.py")
DROP_METHODS=("timeout" "dropmon" "ebpf")
RESULTS_DIR="$(realpath "$SCRIPT_DIR/results")"
export CONF
export CONFGEN
export DROP_METHODS

docker_clean() {
    dockerfiles_dir="$(realpath "$(dirname "${BASH_SOURCE[0]}")"/../Dockerfiles)"
    make -C "$dockerfiles_dir" clean
}

cleanup() {
    set +e
    export err
    err=0
    num_tries=0
    max_tries=20 # loops
    timeout=0.5  # sec/loop

    while true; do
        unfinished_cntrs="$(docker ps -a -q)"
        if [[ -z "$unfinished_cntrs" ]]; then
            break
        elif [[ $num_tries -lt $max_tries ]]; then
            msg "num_tries: $num_tries"
            sleep $timeout
            num_tries=$((num_tries + 1))
        else
            warn "Force cleanup"
            make -C "$PROJECT_DIR/Dockerfiles" clean
            sudo pkill -9 neo
            err=1
            break
        fi
    done
    set -e
}

run() {
    name="$1"
    procs="$2"
    drop="$3"
    infile="$4"
    outdir="$RESULTS_DIR/$name"
    shift 4
    args=("$@")

    msg "Verifying $name"
    sudo "$NEO" -f -j "$procs" -d "$drop" -i "$infile" -o "$outdir" "${args[@]}" >/dev/null
    cleanup
    sudo chown -R "$(id -u):$(id -g)" "$outdir"
    cp "$infile" "$outdir/network.toml"

    # # Repeat until no error occurs
    # while [[ $err -eq 1 ]]; do
    #     msg "Re-verifying $name"
    #     sudo "$NEO" -f -j "$procs" -d "$drop" -i "$infile" -o "$outdir" "${args[@]}" >/dev/null
    #     cleanup
    #     sudo chown -R "$(id -u):$(id -g)" "$outdir"
    #     cp "$infile" "$outdir/network.toml"
    # done
}

int_handler() {
    set +e
    hurt "Interrupted. Closing the running processes and containers..."
    sudo pkill neo
    cleanup
    sudo chown -R "$(id -u):$(id -g)" "$outdir"
    cp "$infile" "$outdir/network.toml"
    hurt "Exit (interrupted)"
    exit 1
}

setup_hugepages() {
    _2M_HUGEPAGE_PATH="/sys/kernel/mm/hugepages/hugepages-2048kB/nr_hugepages"
    _1G_HUGEPAGE_PATH="/sys/kernel/mm/hugepages/hugepages-1048576kB/nr_hugepages"

    # NR_2M_HUGEPAGES="$(cat "$_2M_HUGEPAGE_PATH")"
    # NR_1G_HUGEPAGES="$(cat "$_1G_HUGEPAGE_PATH")"
    # if [[ $NR_2M_HUGEPAGES -eq 0 ]]; then
    #     :
    # fi

    echo 1024 | sudo tee "$_2M_HUGEPAGE_PATH" >/dev/null
    echo 1 | sudo tee "$_1G_HUGEPAGE_PATH" >/dev/null
}

load_vfio_pci_module() {
    MOD=vfio_pci
    if ! lsmod | grep "$MOD" >/dev/null; then
        sudo modprobe "$MOD"
    fi
}

_main() {
    mkdir -p "$RESULTS_DIR"

    while :; do
        case "${1-}" in
        -d | --debug)
            # https://github.com/google/sanitizers/wiki/AddressSanitizerFlags#run-time-flags
            ASAN_OPTIONS="${ASAN_OPTIONS:-verbosity=1:debug=true}"
            ;;
        --)
            shift
            break
            ;;
        -?*) die "Unknown option: ${1-}" ;;
        *) break ;;
        esac
        shift
    done

    export ASAN_OPTIONS
    trap int_handler SIGINT
}

_main "$@"
