import os
import sys
import argparse
import random

sys.path.append(os.path.join(os.path.dirname(__file__), '../../src'))
from config import *
from dataparse import *



def confgen(topo_file_name, emulated_nodes_count):
    config = Config()
    ns = NetSynthesizer(topo_file_name)
    random.seed()
    emulated=random.choices(range(len(ns.node_to_interfaces)), k=emulated_nodes_count)

    for i,n in enumerate(ns.node_to_interfaces):
        if i not in emulated:
            newnode = Node(n,
                           type="model")
        else:
            newnode = DockerNode(n,
                                  image="kyechou/linux-router:latest",
                                  working_dir='/',
                                  command=['/start.sh'])

        for i in ns.node_to_interfaces[n]:
            newnode.add_interface(Interface(i[0], str(i[1])))
        for r in ns.rules[n]:
            newnode.add_static_route(Route(str(r[0]), str(r[1])))
        config.add_node(newnode)



    for l in ns.links:
        config.add_link(Link(l[0], l[1], l[2], l[3]))

    config.output_toml()
    ns.visualize()


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('-t',
                        '--topology',
                        type=str,
                        help='Topology file name')
    parser.add_argument('-e',
                        '--emulated',
                        type=int,
                        help='Number of emulated nodes')
    args = parser.parse_args()
    confgen(args.topology, args.emulated)


if __name__ == '__main__':
    main()
