#!/usr/bin/python3

import os
import sys
import argparse

sys.path.append(os.path.join(os.path.dirname(__file__), '../../src'))
from config import *


def confgen(num_backends):
    config = Config()

    # Add the host node.
    host = Node('host')
    host.add_interface(Interface('eth0', '10.0.0.1/24'))
    host.add_static_route(Route('8.0.0.0/24', '10.0.0.2'))
    config.add_node(host)

    # Add the maglev LB node.
    lb_cmd = ['/nf', '--no-huge']
    for b in range(num_backends):
        lb_cmd.append('--vdev=net_tap{0},iface=eth{0}'.format(b))
    lb_cmd.append('--vdev=net_tap{0},iface=eth{0}'.format(num_backends))
    lb = DockerNode('lb',
                    image='kyechou/klint-maglev:latest',
                    working_dir='/',
                    dpdk=True,
                    command=lb_cmd)
    for b in range(num_backends):
        lb.add_interface(Interface('eth{}'.format(b), '9.0.{}.2/24'.format(b)))
    lb.add_interface(Interface('eth{}'.format(num_backends), '10.0.0.2/24'))
    lb.add_sysctl('net.ipv4.conf.all.forwarding', '1')
    config.add_node(lb)

    # Add the client-side router node for reply packets.
    r_client = Node('r-client')
    for b in range(num_backends):
        r_client.add_interface(
            Interface('eth{}'.format(b), '9.1.{}.2/24'.format(b)))
    r_client.add_interface(
        Interface('eth{}'.format(num_backends), '10.0.0.3/24'))
    config.add_node(r_client)

    # Connect the host, LB, and the client-side router with a switch.
    sw = Node('sw')
    sw.add_interface(Interface('eth0'))
    sw.add_interface(Interface('eth1'))
    sw.add_interface(Interface('eth2'))
    config.add_node(sw)
    config.add_link(Link(sw.name, 'eth0', host.name, 'eth0'))
    config.add_link(
        Link(sw.name, 'eth1', lb.name, 'eth{}'.format(num_backends)))
    config.add_link(
        Link(sw.name, 'eth2', r_client.name, 'eth{}'.format(num_backends)))

    # Add the backend servers and their corresponding routers
    for b in range(num_backends):
        router = Node('r{}'.format(b))
        router.add_interface(Interface('eth0', '8.0.0.2/24'))
        router.add_interface(Interface('eth1', '9.0.{}.1/24'.format(b)))
        router.add_interface(Interface('eth2', '9.1.{}.1/24'.format(b)))
        router.add_static_route(Route('10.0.0.0/24', '9.1.{}.2'.format(b)))
        config.add_node(router)
        server = Node('server{}'.format(b))
        server.add_interface(Interface('eth0', '8.0.0.1/24'))
        server.add_static_route(Route('10.0.0.0/24', '8.0.0.2'))
        config.add_node(server)
        config.add_link(Link(router.name, 'eth0', server.name, 'eth0'))
        config.add_link(Link(router.name, 'eth1', lb.name, 'eth{}'.format(b)))
        config.add_link(
            Link(router.name, 'eth2', r_client.name, 'eth{}'.format(b)))

    # Add invariants
    ## The host can connect to the server via TCP.
    config.add_invariant(
        Reachability(target_node='server[0-9]+',
                     reachable=True,
                     protocol='tcp',
                     src_node='host',
                     dst_ip='8.0.0.1',
                     dst_port=[80],
                     owned_dst_only=True))
    ## The host can connect to the server via UDP.
    config.add_invariant(
        Reachability(target_node='server[0-9]+',
                     reachable=True,
                     protocol='udp',
                     src_node='host',
                     dst_ip='8.0.0.1',
                     dst_port=[80],
                     owned_dst_only=True))
    ## Load balance invariant with a given max dispersion index.
    lb_inv = LoadBalance(target_node='server[0-9]+', max_dispersion_index=100.0)
    for _ in range(2):
        lb_inv.add_connection(
            Connection(protocol='udp', src_node=host.name, dst_ip='8.0.0.1'))
    config.add_invariant(lb_inv)

    # Output as TOML
    config.output_toml()


def main():
    parser = argparse.ArgumentParser(description='13-klint-maglev-lb')
    parser.add_argument('-b',
                        '--backends',
                        type=int,
                        help='Number of backend servers (1-9)')
    args = parser.parse_args()

    if not args.backends or args.backends < 1 or args.backends > 9:
        sys.exit('Invalid number of backends: ' + str(args.backends))

    confgen(args.backends)


if __name__ == '__main__':
    main()
