#!/bin/bash

set -euo pipefail

msg() {
    echo -e "[+] ${1-}" >&2
}

die() {
    echo -e "[!] ${1-}" >&2
    exit 1
}

[ $UID -eq 0 ] && die 'Please run this script without root privilege'

check_depends() {
    for cmd in "$@"; do
        if ! command -v "$cmd" >/dev/null 2>&1; then
            die "'$cmd' not found"
        fi
    done
}

usage() {
    cat <<EOF
[!] Usage: $(basename "${BASH_SOURCE[0]}") [options]

    Options:
    -h, --help          Print this message and exit
EOF
}

parse_params() {
    while :; do
        case "${1-}" in
        -h | --help)
            usage
            exit
            ;;
        -?*) die "Unknown option: $1\n$(usage)" ;;
        *) break ;;
        esac
        shift
    done
}

download_and_extract() {
    url="$1"
    dir="$2"
    filename="$(basename "$url")"
    ext="${filename#*.}"

    mkdir -p "$dir"
    pushd "$dir"
    curl -LO "$url"

    if [[ "$ext" == "tar."* ]]; then
        tar xf "$filename"
    elif [[ "$ext" == "txt.gz" ]]; then
        gunzip --keep "$filename"
    fi
    popd
}

main() {
    parse_params "$@"
    check_depends curl #git make meson ninja

    SCRIPT_DIR="$(dirname "$(realpath "${BASH_SOURCE[0]}")")"
    cd "$SCRIPT_DIR"

    # Rocketfuel
    # https://research.cs.washington.edu/networking/rocketfuel/
    RF_CCH_URL="https://research.cs.washington.edu/networking/rocketfuel/maps/rocketfuel_maps_cch.tar.gz"
    RF_BB_URL="https://research.cs.washington.edu/networking/rocketfuel/maps/weights-dist.tar.gz"

    # Stanford SNAP AS peering network (also from Oregon Route Views)
    # https://snap.stanford.edu/data/as-733.html
    SNAP_AS_733_URL="https://snap.stanford.edu/data/as-733.tar.gz"

    # Stanford SNAP AS peering Oregon Route Views
    # https://snap.stanford.edu/data/Oregon-1.html
    SNAP_OREGON_1_URL="https://snap.stanford.edu/data/oregon1_010331.txt.gz"

    download_and_extract "$RF_CCH_URL" rf-cch
    download_and_extract "$RF_BB_URL" rf-bb
    download_and_extract "$SNAP_AS_733_URL" as-733
    download_and_extract "$SNAP_OREGON_1_URL" oregon-1

    python3 parse-network-dataset.py -i rf-cch -t rocketfuel-cch
    python3 parse-network-dataset.py -i rf-bb -t rocketfuel-bb
    python3 parse-network-dataset.py -i as-733 -t stanford
    python3 parse-network-dataset.py -i oregon-1 -t stanford
}

main "$@"

# vim: set ts=4 sw=4 et:
