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


def confgen(num_backends, firewall_type, lb_type):
    config = Config()

    iptables_fw_rules = """
*filter
:INPUT DROP [0:0]
:FORWARD DROP [0:0]
:OUTPUT ACCEPT [0:0]
-A FORWARD -i eth0 -p tcp --syn -m conntrack --ctstate NEW -j ACCEPT
-A FORWARD -i eth0 -p udp -m conntrack --ctstate NEW -j ACCEPT
-A FORWARD -m conntrack --ctstate RELATED,ESTABLISHED -j ACCEPT
COMMIT
"""

    if lb_type == "ipvs":
        # IPVS mode (masquerading):
        # All backend servers have a different IP address.
        target_service_ip = "10.0.0.2"  # client-facing IP of the LB
    elif lb_type == "klint":
        # Klint maglev LB mode:
        # All backend servers have the same IP address.
        target_service_ip = "8.0.0.1"  # shared backend server IP address
    else:
        raise Exception("Unknown lb type: {}".format(lb_type))

    # Add the host node.
    host = Node("host")
    host.add_interface(Interface("eth0", "10.0.0.1/24"))
    host.add_static_route(Route("8.0.0.0/24", "10.0.0.2"))
    config.add_node(host)

    # Add the LB node.
    lb = None
    if lb_type == "ipvs":
        lb = DockerNode(
            "lb",
            image="kyechou/ipvs:latest",
            working_dir="/",
            reset_delay=1000,
            command=["/start.sh"],
        )
        ipvs_config = "-A -t 10.0.0.2:80 -s mh\n"
        ipvs_config += "-A -u 10.0.0.2:80 -s mh\n"
        for b in range(num_backends):
            lb.add_interface(Interface("eth{}".format(b), "9.1.{}.2/24".format(b)))
            lb.add_static_route(Route("8.1.{}.0/24".format(b), "9.1.{}.1".format(b)))
            ipvs_config += "-a -t 10.0.0.2:80 -r 8.1.{}.1:80 -m\n".format(b)
            ipvs_config += "-a -u 10.0.0.2:80 -r 8.1.{}.1:80 -m\n".format(b)
        lb.add_interface(Interface("eth{}".format(num_backends), "10.0.0.2/24"))
        lb.add_volume("/lib/modules", "/lib/modules")
        lb.add_sysctl("net.ipv4.conf.all.forwarding", "1")
        lb.add_sysctl("net.ipv4.vs.expire_nodest_conn", "1")
        lb.add_env_var("RULES", ipvs_config)
        config.add_node(lb)
    elif lb_type == "klint":
        lb_cmd = ["/nf", "--no-huge"]
        for b in range(num_backends):
            lb_cmd.append("--vdev=net_tap{0},iface=eth{0}".format(b))
        lb_cmd.append("--vdev=net_tap{0},iface=eth{0}".format(num_backends))
        lb = DockerNode(
            "lb",
            image="kyechou/klint-maglev:latest",
            working_dir="/",
            dpdk=True,
            command=lb_cmd,
        )
        for b in range(num_backends):
            lb.add_interface(Interface("eth{}".format(b), "9.1.{}.2/24".format(b)))
        lb.add_interface(Interface("eth{}".format(num_backends), "10.0.0.2/24"))
        lb.add_sysctl("net.ipv4.conf.all.forwarding", "1")
        config.add_node(lb)
    assert type(lb) is DockerNode

    # Add the client-side router node for reply packets.
    r_client = Node("r-client")
    for b in range(num_backends):
        r_client.add_interface(Interface("eth{}".format(b), "9.2.{}.2/24".format(b)))
    r_client.add_interface(Interface("eth{}".format(num_backends), "10.0.0.3/24"))
    config.add_node(r_client)

    # Connect the host, LB, and the client-side router with a switch.
    sw = Node("sw")
    sw.add_interface(Interface("eth0"))
    sw.add_interface(Interface("eth1"))
    sw.add_interface(Interface("eth2"))
    config.add_node(sw)
    config.add_link(Link(sw.name, "eth0", host.name, "eth0"))
    config.add_link(Link(sw.name, "eth1", lb.name, "eth{}".format(num_backends)))
    config.add_link(Link(sw.name, "eth2", r_client.name, "eth{}".format(num_backends)))

    # Add the backend servers and their corresponding routers and firewalls
    for b in range(num_backends):
        router = Node("r{}".format(b))
        router.add_interface(Interface("eth0", "9.0.{}.1/24".format(b)))
        router.add_interface(Interface("eth1", "9.1.{}.1/24".format(b)))
        router.add_interface(Interface("eth2", "9.2.{}.1/24".format(b)))
        if lb_type == "ipvs":
            router.add_static_route(
                Route("8.1.{}.0/24".format(b), "9.0.{}.2".format(b))
            )
            router.add_static_route(Route("10.0.0.0/24", "9.1.{}.2".format(b)))
        elif lb_type == "klint":
            router.add_static_route(Route("8.0.0.0/24", "9.0.{}.2".format(b)))
            router.add_static_route(Route("10.0.0.0/24", "9.2.{}.2".format(b)))
        config.add_node(router)

        if firewall_type == "iptables":
            fw = DockerNode(
                "fw{}".format(b),
                image="kyechou/iptables:latest",
                working_dir="/",
                command=["/start.sh"],
            )
            fw.add_env_var("RULES", iptables_fw_rules)
            fw.add_sysctl("net.ipv4.conf.all.rp_filter", "0")
            fw.add_sysctl("net.ipv4.conf.default.rp_filter", "0")
        elif firewall_type == "klint":
            fw = DockerNode(
                "fw{}".format(b),
                image="kyechou/klint-firewall:latest",
                working_dir="/",
                dpdk=True,
                command=[
                    "/nf",
                    "--no-huge",
                    "--vdev=net_tap0,iface=eth0",
                    "--vdev=net_tap1,iface=eth1",
                ],
            )
        else:
            raise Exception("Unknown firewall type: {}".format(firewall_type))

        fw.add_interface(Interface("eth0", "9.0.{}.2/24".format(b)))
        if lb_type == "ipvs":
            fw.add_interface(Interface("eth1", "8.1.{}.2/24".format(b)))
        elif lb_type == "klint":
            fw.add_interface(Interface("eth1", "8.0.0.2/24"))
        fw.add_static_route(Route("10.0.0.0/24", "9.0.{}.1".format(b)))
        config.add_node(fw)

        server = Node("server{}".format(b))
        if lb_type == "ipvs":
            server.add_interface(Interface("eth0", "8.1.{}.1/24".format(b)))
            server.add_static_route(Route("10.0.0.0/24", "8.1.{}.2".format(b)))
        elif lb_type == "klint":
            server.add_interface(Interface("eth0", "8.0.0.1/24"))
            server.add_static_route(Route("10.0.0.0/24", "8.0.0.2"))
        config.add_node(server)
        config.add_link(Link(router.name, "eth0", fw.name, "eth0"))
        config.add_link(Link(router.name, "eth1", lb.name, "eth{}".format(b)))
        config.add_link(Link(router.name, "eth2", r_client.name, "eth{}".format(b)))
        config.add_link(Link(fw.name, "eth1", server.name, "eth0"))

    # Add invariants
    ## The host can connect to the server via TCP.
    config.add_invariant(
        Reachability(
            target_node="server[0-9]+",
            reachable=True,
            protocol="tcp",
            src_node="host",
            dst_ip=target_service_ip,
            dst_port=[80],
        )
    )
    ## The host can connect to the server via UDP.
    config.add_invariant(
        Reachability(
            target_node="server[0-9]+",
            reachable=True,
            protocol="udp",
            src_node="host",
            dst_ip=target_service_ip,
            dst_port=[80],
        )
    )
    ## Reply to past requests can reach the original sender via TCP.
    config.add_invariant(
        ReplyReachability(
            target_node="server[0-9]+",
            reachable=True,
            protocol="tcp",
            src_node="host",
            dst_ip=target_service_ip,
            dst_port=[80],
        )
    )
    ## Reply to past requests can reach the original sender via UDP.
    config.add_invariant(
        ReplyReachability(
            target_node="server[0-9]+",
            reachable=True,
            protocol="udp",
            src_node="host",
            dst_ip=target_service_ip,
            dst_port=[80],
        )
    )

    # Output as TOML
    config.output_toml()


def main():
    parser = argparse.ArgumentParser(description="16-klint-lb-fw")
    parser.add_argument(
        "-b",
        "--backends",
        type=int,
        required=True,
        action="store",
        help="Number of backend servers (1-9)",
    )
    parser.add_argument(
        "--firewall",
        type=str,
        choices=["iptables", "klint"],
        required=True,
        help="Types of firewall",
    )
    parser.add_argument(
        "--lb",
        type=str,
        choices=["ipvs", "klint"],
        required=True,
        help="Types of load balancer",
    )
    args = parser.parse_args()

    if not args.backends or args.backends < 1 or args.backends > 9:
        sys.exit("Invalid number of backends: " + str(args.backends))

    confgen(args.backends, args.firewall, args.lb)


if __name__ == "__main__":
    main()
