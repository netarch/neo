#!/usr/bin/python3

import argparse
import os
import sys

sys.path.append(os.path.join(os.path.dirname(__file__), "../.."))
from src.config import (
    Config,
    Connection,
    DockerNode,
    Interface,
    Link,
    LoadBalance,
    Node,
    Route,
)


def confgen(lbs, servers, algorithm):
    config = Config()

    ## add the Internet node and r1
    internet_node = Node("internet")
    internet_node.add_interface(Interface("eth0", "8.0.0.2/24"))
    internet_node.add_static_route(Route("0.0.0.0/0", "8.0.0.1"))
    r1 = Node("r1")
    r1.add_interface(Interface("eth0", "8.0.0.1/24"))
    for lb in range(1, lbs + 1):
        r1.add_interface(Interface("eth%d" % lb, "8.0.%d.1/24" % lb))
        r1.add_static_route(Route("9.%d.0.0/16" % lb, "8.0.%d.2" % lb))
    config.add_node(internet_node)
    config.add_node(r1)
    config.add_link(Link("internet", "eth0", "r1", "eth0"))

    ## add the load balancers, switches, and servers
    for lb in range(1, lbs + 1):
        load_balancer = DockerNode(
            "lb%d" % lb,
            image="kyechou/ipvs:latest",
            working_dir="/",
            reset_delay=1000,  # usec
            command=["/start.sh"],
        )
        load_balancer.add_interface(Interface("eth0", "8.0.%d.2/24" % lb))
        load_balancer.add_interface(Interface("eth1", "9.%d.0.1/16" % lb))
        load_balancer.add_static_route(Route("0.0.0.0/0", "8.0.%d.1" % lb))
        # This is needed because `ipvsadm` may need to load the `ip_vs` kernel
        # module if it's not loaded already.
        load_balancer.add_volume("/lib/modules", "/lib/modules")
        load_balancer.add_sysctl("net.ipv4.vs.expire_nodest_conn", "1")
        lb_config = ""
        lb_config += "-A -u 8.0.%d.2:80 -s %s" % (lb, algorithm)
        if algorithm == "sh":
            lb_config += " -b sh-port"  # add source port into hash computation
        lb_config += "\n"
        sw = Node("sw%d" % lb)
        sw.add_interface(Interface("eth0"))
        config.add_node(load_balancer)
        config.add_node(sw)
        config.add_link(Link("r1", "eth%d" % lb, load_balancer.name, "eth0"))
        config.add_link(Link(load_balancer.name, "eth1", sw.name, "eth0"))
        for srv in range(1, servers + 1):
            sw.add_interface(Interface("eth%d" % srv))
            server = Node("server%d.%d" % (lb, srv))
            third = ((srv + 1) // 256) % 256
            last = (srv + 1) % 256
            server.add_interface(Interface("eth0", "9.%d.%d.%d/24" % (lb, third, last)))
            server.add_static_route(Route("0.0.0.0/0", "9.%d.0.1" % lb))
            lb_config += "-a -u 8.0.%d.2:80 -r 9.%d.%d.%d:80 -m\n" % (
                lb,
                lb,
                third,
                last,
            )
            config.add_node(server)
            config.add_link(Link(sw.name, "eth%d" % srv, server.name, "eth0"))
        load_balancer.add_env_var("RULES", lb_config)

    ## add invariants
    for lb in range(1, 2):
        inv = LoadBalance(
            target_node="server%d\\.[0-9]+" % lb, max_dispersion_index=2.5
        )
        num_conns = servers
        for repeat in range(num_conns):
            inv.add_connection(
                Connection(
                    protocol="udp",
                    src_node=internet_node.name,
                    dst_ip="8.0.%d.2" % lb,
                    src_port=50000 + repeat,
                    dst_port=[80],
                )
            )
        config.add_invariant(inv)

    ## output as TOML
    config.output_toml()


def main():
    parser = argparse.ArgumentParser(description="03-load-balancing")
    parser.add_argument(
        "-l", "--lbs", type=int, help="Number of load balancers (HTTP servers)"
    )
    parser.add_argument(
        "-s", "--servers", type=int, help="Number of servers behind each LB"
    )
    parser.add_argument(
        "-a",
        "--algorithm",
        choices=["rr", "sh", "dh", "mh", "lc"],
        help="Load-balancing algorithm",
    )
    arg = parser.parse_args()

    if not arg.lbs or arg.lbs > 255:
        sys.exit("Invalid number of load balancers: " + str(arg.lbs))
    if not arg.servers or arg.servers > 65533:
        sys.exit("Invalid number of servers: " + str(arg.servers))

    confgen(arg.lbs, arg.servers, arg.algorithm)


if __name__ == "__main__":
    main()
