#!/bin/bash

set -euo pipefail

msg() {
    echo -e "[+] ${1-}" >&2
}

die() {
    echo -e "[!] ${1-}" >&2
    exit 1
}

if [ $UID -eq 0 ]; then
    die 'Please run this script without root privilege'
fi

images=(
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
