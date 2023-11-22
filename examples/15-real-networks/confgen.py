import os
import sys
import argparse
import random

sys.path.append(os.path.join(os.path.dirname(__file__), '../../src'))
from config import *
from dataparse import *


def confgen(topo_file_name, emu_percentage, max_inv_endpoints):
    config = Config()
    ns = NetSynthesizer(topo_file_name)
    node_count = len(ns.node_to_interfaces)
    random.seed(42)  # Fixed seed for reproducibility
    emulated = random.sample(range(node_count),
                             k=int(node_count * emu_percentage / 100))

    for i, n in enumerate(ns.node_to_interfaces):
        if i not in emulated:
            newnode = Node(n, type="model")
        else:
            newnode = DockerNode(n,
                                 image="kyechou/linux-router:latest",
                                 working_dir='/',
                                 command=['/start.sh'])
            newnode.add_sysctl('net.ipv4.conf.all.forwarding', '1')
            newnode.add_sysctl('net.ipv4.conf.all.rp_filter', '0')
            newnode.add_sysctl('net.ipv4.conf.default.rp_filter', '0')

        for i in ns.node_to_interfaces[n]:
            newnode.add_interface(Interface(i[0], str(i[1]) + "/30"))
        for r in ns.rules[n]:
            newnode.add_static_route(Route(str(r[0]), str(r[1])))
        config.add_node(newnode)

    for l in ns.links:
        config.add_link(Link(l[0], l[1], l[2], l[3]))

    leaves = ns.leaves()
    leaves = random.sample(leaves, k=min(len(leaves), max_inv_endpoints))
    for u in leaves:
        for v in leaves:
            if u != v:
                dst = str(ns.node_to_interfaces[v][0][1])
                config.add_invariant(
                    Reachability(target_node=v,
                                 reachable=True,
                                 protocol='tcp',
                                 src_node=u,
                                 dst_ip=dst,
                                 dst_port=[80]))

    config.output_toml()


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('-t',
                        '--topo',
                        type=str,
                        help='Topology file name',
                        required=True)
    parser.add_argument('-e',
                        '--emulated',
                        type=int,
                        help='Percentage of emulated nodes',
                        required=True)
    parser.add_argument('-i',
                        '--inv-endpoints',
                        type=int,
                        help='Maximum number of leaves to be tested',
                        required=True)
    arg = parser.parse_args()

    base_dir = os.path.abspath(os.path.dirname(__file__))
    in_filename = os.path.join(base_dir, 'data', arg.topo)
    confgen(in_filename, arg.emulated, arg.inv_endpoints)


if __name__ == '__main__':
    main()
