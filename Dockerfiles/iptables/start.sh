#!/bin/sh

RULES_FILE="iptables.rules"

sig_handler() {
    kill -KILL -- 0 # all processes in the current process group
    exit 0
}

cat <<EOF >"$RULES_FILE"
*filter
:INPUT DROP [0:0]
:FORWARD DROP [0:0]
:OUTPUT ACCEPT [0:0]
-A INPUT -i eth1 -j ACCEPT
-A INPUT -i eth2 -j ACCEPT
-A INPUT -m conntrack --ctstate RELATED,ESTABLISHED -j ACCEPT
-A FORWARD -i eth1 -j ACCEPT
-A FORWARD -i eth2 -j ACCEPT
-A FORWARD -m conntrack --ctstate RELATED,ESTABLISHED -j ACCEPT
COMMIT
EOF

iptables -F
iptables -Z
iptables-restore "$RULES_FILE"
trap sig_handler HUP INT QUIT ABRT TERM
sleep infinity &
wait
