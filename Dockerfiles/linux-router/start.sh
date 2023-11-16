#!/bin/sh

set -eu

sig_handler() {
    kill -KILL -- 0 # all processes in the current process group
    exit 0
}

trap sig_handler HUP INT QUIT ABRT TERM
sleep infinity &
wait
