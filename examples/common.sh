#!/bin/bash
#
# Common shell utilities for experiment scripts
#
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

NEO="$(realpath "$(dirname "${BASH_SOURCE[0]}")"/../build/neo)"
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

run() {
    name="$1"
    procs="$2"
    drop="$3"
    infile="$4"
    outdir="$RESULTS_DIR/$name"

    msg "Verifying $name"
    sudo "$NEO" -f -j "$procs" -d "$drop" -i "$infile" -o "$outdir"
    sudo chown -R "$(id -u):$(id -g)" "$outdir"
    cp "$infile" "$outdir/network.toml"

    # Clean up containers and processes
    sleep 5
    set +e
    cntrs="$(docker ps -a -q)"
    if [[ -n "$cntrs" ]]; then
        docker_clean
        sudo pkill -9 neo
        warn "Containers were not cleared up. Something went wrong."
    fi
    set -e
}

_main() {
    mkdir -p "$RESULTS_DIR"

    while :; do
        case "${1-}" in
        -d | --debug)
            # https://github.com/google/sanitizers/wiki/AddressSanitizerFlags#run-time-flags
            ASAN_OPTIONS="${ASAN_OPTIONS:-verbosity=1:debug=true}"
            ;;
        --) shift; break ;;
        -?*) die "Unknown option: ${1-}" ;;
        *) break ;;
        esac
        shift
    done

    export ASAN_OPTIONS
}

_main "$@"

# extract_per_session_stats() {
#     dir="$1"
#     name="$(basename "$dir")"
#     main_log="$dir/main.log"
#     [[ ! -d "$dir" || ! -f "$main_log" ]] && die "Invalid directory: $dir"

#     #
#     # main process parameters
#     #
#     procs=0
#     fault=N
#     dropmon=N

#     oldIFS=$IFS
#     IFS="."
#     for parameter in $name; do
#         if [[ "$parameter" == "DOP-"* ]]; then
#             procs="${parameter//DOP-/}"
#         elif [[ "$parameter" == "fault" ]]; then
#             fault=Y
#         elif [[ "$parameter" == "dropmon" ]]; then
#             dropmon=Y
#         fi
#     done
#     IFS=$oldIFS

#     #
#     # policy process measurements
#     #
#     # fault, dropmon, parallel processes, nodes, links, Policy, Connection ECs, Time (microseconds), Memory (kilobytes)
#     for policy in "$dir"/*; do
#         [[ ! -d "$policy" ]] && continue

#         # policy-wide stats
#         policy_result="$(tail -n1 "$policy/policy.stats.csv")"
#         echo "$fault, $dropmon, $procs, $policy_result"
#     done
# }

# extract_per_session_latency() {
#     dir="$1"
#     name="$(basename "$dir")"
#     main_log="$dir/main.log"
#     [[ ! -d "$dir" || ! -f "$main_log" ]] && die "Invalid directory: $dir"

#     #
#     # main process parameters
#     #
#     procs=0
#     fault=N
#     dropmon=N

#     oldIFS=$IFS
#     IFS="."
#     for parameter in $name; do
#         if [[ "$parameter" == "DOP-"* ]]; then
#             procs="${parameter//DOP-/}"
#         elif [[ "$parameter" == "fault" ]]; then
#             fault=Y
#         elif [[ "$parameter" == "dropmon" ]]; then
#             dropmon=Y
#         fi
#     done
#     IFS=$oldIFS

#     #
#     # policy process measurements
#     #
#     for policy in "$dir"/*; do
#         [[ ! -d "$policy" ]] && continue

#         # policy-wide stats
#         policy_result="$(tail -n1 "$policy/policy.stats.csv")"
#         num_nodes="$(echo "$policy_result" | cut -d ' ' -f 1 | sed 's/,//')"
#         num_links="$(echo "$policy_result" | cut -d ' ' -f 2 | sed 's/,//')"
#         policy_no="$(echo "$policy_result" | cut -d ' ' -f 3 | sed 's/,//')"
#         num_CECs="$(echo "$policy_result" | cut -d ' ' -f 4 | sed 's/,//')"

#         # output all latency measurements
#         # fault, dropmon, parallel processes, nodes, links, Policy, Connection ECs, <latencies>
#         for csv in "$policy"/*.csv; do
#             [[ "$(basename "$csv")" == "policy.stats.csv" ]] && continue

#             tail -n+4 "$csv" | sed -e "s/^/$fault, $dropmon, $procs, $num_nodes, $num_links, $policy_no, $num_CECs, /"
#         done

#     done
# }

# extract_stats() {
#     results_dir="$1"
#     [[ ! -d "$results_dir" ]] && die "Invalid directory: $results_dir"

#     echo 'fault, dropmon, parallel processes, nodes, links, Policy, Connection ECs, Time (microseconds), Memory (kilobytes)'
#     for dir in "$results_dir"/*; do
#         [[ ! -d "$dir" ]] && continue
#         extract_per_session_stats "$dir"
#     done
# }

# extract_latency() {
#     results_dir="$1"
#     [[ ! -d "$results_dir" ]] && die "Invalid directory: $results_dir"

#     echo -n 'fault, dropmon, parallel processes, nodes, links, Policy, Connection ECs, '
#     echo -n 'Overall latency t1 (nsec), Overall latency (nsec), '
#     echo -n 'Rewind latency t1 (nsec), Rewind latency (nsec), Rewind injection count, '
#     echo -n 'Packet latency t1 (nsec), Packet latency (nsec), Timeout (nsec), '
#     echo    'Kernel drop latency (nsec)'
#     for dir in "$results_dir"/*; do
#         [[ ! -d "$dir" ]] && continue
#         extract_per_session_latency "$dir"
#     done
# }
