import os
import sys
import argparse



sys.path.append(os.path.join(os.path.dirname(__file__), '../../src'))
from config import *
from dataparse import *

def confgen(links):
    config=Config()
    ns = NetSynthesizer(links)
    for n in ns.node_to_interfaces:
        newnode=Node(n)
        for i in ns.node_to_interfaces[n]:
            newnode.add_interface(Interface(i[0],str(i[1])))
        for r in ns.rules[n]:
            newnode.add_static_route(Route(str(r[0]),str(r[1])))
        config.add_node(newnode)

    for l in ns.links:
        config.add_link(Link(l[0],l[1],l[2],l[3]))


    config.output_toml()
    ns.visualize()
def main():
    parser=argparse.ArgumentParser()
    parser.add_argument('-l',
                        '--links',
                        type=str)
    arg=parser.parse_args()
    confgen(arg.links)

if __name__ == '__main__':
    main()

