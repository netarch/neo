#!/bin/sh

set -eu

sig_handler() {
    kill -KILL -- 0 # all processes in the current process group
    exit 0
}

if [ -z "${RULES+x}" ]; then
    echo '[!] env RULES is unset' >&2
    exit 1
fi

iptables -F
iptables -Z
echo "$RULES" | iptables-restore
trap sig_handler HUP INT QUIT ABRT TERM
sleep infinity &
wait
