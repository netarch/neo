#!/usr/bin/python3

import argparse
import os
import sys

sys.path.append(os.path.join(os.path.dirname(__file__), "../.."))
from src.config import (
    Config,
    DockerNode,
    Interface,
    Link,
    Node,
    Reachability,
    ReplyReachability,
    Route,
)


def confgen(subnets, hosts, fault):
    config = Config()

    ## firewall rules
    fw_rules = """
*filter
:INPUT DROP [0:0]
:FORWARD DROP [0:0]
:OUTPUT ACCEPT [0:0]
-A FORWARD -i eth0 -d 12.0.0.0/8 -j ACCEPT
-A FORWARD -i eth0 -d 11.0.0.0/8 -m conntrack --ctstate RELATED,ESTABLISHED -j ACCEPT
-A FORWARD -i eth1 -s 12.0.0.0/8 -j ACCEPT
-A FORWARD -i eth1 -s 11.0.0.0/8 -j ACCEPT
COMMIT
"""
    fw_bad_rules = """
*filter
:INPUT DROP [0:0]
:FORWARD ACCEPT [0:0]
:OUTPUT ACCEPT [0:0]
-A FORWARD -i eth0 -d 12.0.0.0/8 -j DROP
-A FORWARD -i eth1 -s 12.0.0.0/8 -j DROP
-A FORWARD -i eth1 -s 11.0.0.0/8 -m conntrack --ctstate NEW -j DROP
COMMIT
"""

    ## add the Internet node, firewall, and gateway
    internet_node = Node("internet")
    internet_node.add_interface(Interface("eth0", "8.0.0.1/24"))
    internet_node.add_static_route(Route("9.0.0.0/24", "8.0.0.2"))
    internet_node.add_static_route(Route("10.0.0.0/8", "8.0.0.2"))
    internet_node.add_static_route(Route("11.0.0.0/8", "8.0.0.2"))
    internet_node.add_static_route(Route("12.0.0.0/8", "8.0.0.2"))
    config.add_node(internet_node)
    fw = DockerNode(
        "fw", image="kyechou/iptables:latest", working_dir="/", command=["/start.sh"]
    )
    fw.add_interface(Interface("eth0", "8.0.0.2/24"))
    fw.add_interface(Interface("eth1", "9.0.0.1/24"))
    fw.add_static_route(Route("0.0.0.0/0", "8.0.0.1"))
    fw.add_static_route(Route("10.0.0.0/8", "9.0.0.2"))
    fw.add_static_route(Route("11.0.0.0/8", "9.0.0.2"))
    fw.add_static_route(Route("12.0.0.0/8", "9.0.0.2"))
    fw.add_env_var("RULES", fw_rules if not fault else fw_bad_rules)
    fw.add_sysctl("net.ipv4.conf.all.forwarding", "1")
    fw.add_sysctl("net.ipv4.conf.all.rp_filter", "0")
    fw.add_sysctl("net.ipv4.conf.default.rp_filter", "0")
    config.add_node(fw)
    gw = Node("gw")
    gw.add_interface(Interface("eth0", "9.0.0.2/24"))
    for subnet in range(subnets):  # add "public"-connecting interfaces
        intf = Interface("eth%d" % (subnet + 1), "12.%d.0.1/16" % subnet)
        gw.add_interface(intf)
    for subnet in range(subnets):  # add "private"-connecting interfaces
        intf = Interface("eth%d" % (subnet + 1 + subnets), "11.%d.0.1/16" % subnet)
        gw.add_interface(intf)
    for subnet in range(subnets):  # add "quarantined"-connecting interfaces
        intf = Interface("eth%d" % (subnet + 1 + subnets * 2), "10.%d.0.1/16" % subnet)
        gw.add_interface(intf)
    gw.add_static_route(Route("0.0.0.0/0", "9.0.0.1"))
    config.add_node(gw)
    config.add_link(Link("internet", "eth0", "fw", "eth0"))
    config.add_link(Link("fw", "eth1", "gw", "eth0"))

    ## add nodes and links in the public subnets
    for subnet in range(subnets):
        sw = Node("public%d-sw" % subnet)
        sw.add_interface(Interface("eth0"))
        for i in range(1, hosts + 1):
            sw.add_interface(Interface("eth%d" % i))
        config.add_node(sw)
        config.add_link(Link(sw.name, "eth0", "gw", "eth%d" % (subnet + 1)))
        for i in range(1, hosts + 1):
            host = Node("public%d-host%d" % (subnet, i - 1))
            second_last = ((i + 1) // 256) % 256
            last = (i + 1) % 256
            host.add_interface(
                Interface("eth0", "12.%d.%d.%d/16" % (subnet, second_last, last))
            )
            host.add_static_route(Route("0.0.0.0/0", "12.%d.0.1" % subnet))
            config.add_node(host)
            config.add_link(Link(host.name, "eth0", sw.name, "eth%d" % i))

    ## add nodes and links in the private subnets
    for subnet in range(subnets):
        sw = Node("private%d-sw" % subnet)
        sw.add_interface(Interface("eth0"))
        for i in range(1, hosts + 1):
            sw.add_interface(Interface("eth%d" % i))
        config.add_node(sw)
        config.add_link(Link(sw.name, "eth0", "gw", "eth%d" % (subnet + 1 + subnets)))
        for i in range(1, hosts + 1):
            host = Node("private%d-host%d" % (subnet, i - 1))
            second_last = ((i + 1) // 256) % 256
            last = (i + 1) % 256
            host.add_interface(
                Interface("eth0", "11.%d.%d.%d/16" % (subnet, second_last, last))
            )
            host.add_static_route(Route("0.0.0.0/0", "11.%d.0.1" % subnet))
            config.add_node(host)
            config.add_link(Link(host.name, "eth0", sw.name, "eth%d" % i))

    ## add nodes and links in the quarantined subnets
    for subnet in range(subnets):
        sw = Node("quarantined%d-sw" % subnet)
        sw.add_interface(Interface("eth0"))
        for i in range(1, hosts + 1):
            sw.add_interface(Interface("eth%d" % i))
        config.add_node(sw)
        config.add_link(
            Link(sw.name, "eth0", "gw", "eth%d" % (subnet + 1 + subnets * 2))
        )
        for i in range(1, hosts + 1):
            host = Node("quarantined%d-host%d" % (subnet, i - 1))
            second_last = ((i + 1) // 256) % 256
            last = (i + 1) % 256
            host.add_interface(
                Interface("eth0", "10.%d.%d.%d/16" % (subnet, second_last, last))
            )
            host.add_static_route(Route("0.0.0.0/0", "10.%d.0.1" % subnet))
            config.add_node(host)
            config.add_link(Link(host.name, "eth0", sw.name, "eth%d" % i))

    ## add invariants
    # public subnets can initiate connections to the outside world
    config.add_invariant(
        Reachability(
            target_node="internet",
            reachable=True,
            protocol="tcp",
            src_node="public.*-host.*",
            dst_ip="8.0.0.1",
            dst_port=[80],
            owned_dst_only=True,
        )
    )
    # public subnets can accept connections from the outside world
    config.add_invariant(
        Reachability(
            target_node="(public.*-host.*)|gw",
            reachable=True,
            protocol="tcp",
            src_node="internet",
            dst_ip="12.0.0.0/8",
            dst_port=[80],
            owned_dst_only=True,
        )
    )
    # private subnets can initiate connections to the outside world and replies
    # from the outside world can reach the private subnets
    config.add_invariant(
        ReplyReachability(
            target_node="internet",
            reachable=True,
            protocol="tcp",
            src_node="private.*-host.*",
            dst_ip="8.0.0.1",
            dst_port=[80],
            owned_dst_only=True,
        )
    )
    # private subnets can't accept connections from the outside world
    config.add_invariant(
        Reachability(
            target_node="(private.*-host.*)|gw",
            reachable=False,
            protocol="tcp",
            src_node="internet",
            dst_ip="11.0.0.0/8",
            dst_port=[80],
            owned_dst_only=True,
        )
    )
    # quarantined subnets can't initiate connections to the outside world
    config.add_invariant(
        Reachability(
            target_node="internet",
            reachable=False,
            protocol="tcp",
            src_node="quarantined.*-host.*",
            dst_ip="8.0.0.1",
            dst_port=[80],
            owned_dst_only=True,
        )
    )
    # quarantined subnets can't accept connections from the outside world
    config.add_invariant(
        Reachability(
            target_node="(quarantined.*-host.*)|gw",
            reachable=False,
            protocol="tcp",
            src_node="internet",
            dst_ip="10.0.0.0/8",
            dst_port=[80],
            owned_dst_only=True,
        )
    )

    ## output as TOML
    config.output_toml()


def main():
    parser = argparse.ArgumentParser(description="01-subnet-isolation")
    parser.add_argument(
        "-s", "--subnets", type=int, help="Number of subnets in each policy zone"
    )
    parser.add_argument(
        "-H", "--hosts", type=int, help="Number of hosts in each subnet"
    )
    parser.add_argument(
        "-f", "--fault", action="store_true", default=False, help="Use faulty rules"
    )
    arg = parser.parse_args()

    if not arg.subnets or arg.subnets > 256:
        sys.exit("Invalid number of subnets: " + str(arg.subnets))
    if not arg.hosts or arg.hosts > 65533:
        sys.exit("Invalid number of hosts: " + str(arg.hosts))

    confgen(arg.subnets, arg.hosts, arg.fault)


if __name__ == "__main__":
    main()
