import os
import sys
import argparse

sys.path.append(os.path.join(os.path.dirname(__file__), '../../src'))
from config import *
from dataparse import *


def confgen(topo_file_name):
    config = Config()

    ns = NetSynthesizer(topo_file_name)
    for n in ns.node_to_interfaces:
        newnode = Node(n)
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
    args = parser.parse_args()
    confgen(args.topology)


if __name__ == '__main__':
    main()
