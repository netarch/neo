#!/usr/bin/python3

import sys
import toml
import argparse
from conf_classes import *

def confgen(subnets, hosts):
    network = Network()
    policies = Policies()

    ## add the Internet node, firewall, and gateway
    internet_node = Node('internet')
    internet_node.add_interface(Interface('eth0', '8.0.0.1/24'))
    internet_node.add_static_route(Route('9.0.0.0/24', '8.0.0.2'))
    internet_node.add_static_route(Route('10.0.0.0/8', '8.0.0.2'))
    internet_node.add_static_route(Route('11.0.0.0/8', '8.0.0.2'))
    internet_node.add_static_route(Route('12.0.0.0/8', '8.0.0.2'))
    network.add_node(internet_node)
    fw = Middlebox('fw', 'netns', 'netfilter')
    fw.add_interface(Interface('eth0', '8.0.0.2/24'))
    fw.add_interface(Interface('eth1', '9.0.0.1/24'))
    fw.add_static_route(Route('0.0.0.0/0', '8.0.0.1'))
    fw.add_static_route(Route('10.0.0.0/8', '9.0.0.2'))
    fw.add_static_route(Route('11.0.0.0/8', '9.0.0.2'))
    fw.add_static_route(Route('12.0.0.0/8', '9.0.0.2'))
    fw.add_config('rp_filter', 0)
    fw.add_config('rules', """
*filter
:INPUT DROP [0:0]
:FORWARD DROP [0:0]
:OUTPUT ACCEPT [0:0]
-A FORWARD -i eth0 -d 12.0.0.0/8 -j ACCEPT
-A FORWARD -i eth0 -d 11.0.0.0/8 -m conntrack --ctstate RELATED,ESTABLISHED -j ACCEPT
-A FORWARD -i eth1 -s 12.0.0.0/8 -j ACCEPT
-A FORWARD -i eth1 -s 11.0.0.0/8 -j ACCEPT
COMMIT
""")
    network.add_node(fw)
    gw = Node('gw')
    gw.add_interface(Interface('eth0', '9.0.0.2/24'))
    for subnet in range(subnets):   # add "public"-connecting interfaces
        intf = Interface('eth%d' % (subnet + 1),
                         '12.%d.0.1/16' % subnet)
        gw.add_interface(intf)
    for subnet in range(subnets):   # add "private"-connecting interfaces
        intf = Interface('eth%d' % (subnet + 1 + subnets),
                         '11.%d.0.1/16' % subnet)
        gw.add_interface(intf)
    for subnet in range(subnets):   # add "quarantined"-connecting interfaces
        intf = Interface('eth%d' % (subnet + 1 + subnets * 2),
                         '10.%d.0.1/16' % subnet)
        gw.add_interface(intf)
    gw.add_static_route(Route('0.0.0.0/0', '9.0.0.1'))
    network.add_node(gw)
    network.add_link(Link('internet', 'eth0', 'fw', 'eth0'))
    network.add_link(Link('fw', 'eth1', 'gw', 'eth0'))

    ## add nodes and links in the public subnets
    for subnet in range(subnets):
        sw = Node('public%d-sw' % subnet)
        sw.add_interface(Interface('eth0'))
        for i in range(1, hosts + 1):
            sw.add_interface(Interface('eth%d' % i))
        network.add_node(sw)
        network.add_link(Link(sw.name, 'eth0', 'gw', 'eth%d' % (subnet + 1)))
        for i in range(1, hosts + 1):
            host = Node('public%d-host%d' % (subnet, i))
            second_last_octet = ((i + 1) // 256) % 256
            last_octet = (i + 1) % 256
            host.add_interface(Interface('eth0',
                '12.%d.%d.%d/16' % (subnet, second_last_octet, last_octet)))
            host.add_static_route(Route('0.0.0.0/0', '12.%d.0.1' % subnet))
            network.add_node(host)
            network.add_link(Link(host.name, 'eth0', sw.name, 'eth%d' % i))

    ## add nodes and links in the private subnets
    for subnet in range(subnets):
        sw = Node('private%d-sw' % subnet)
        sw.add_interface(Interface('eth0'))
        for i in range(1, hosts + 1):
            sw.add_interface(Interface('eth%d' % i))
        network.add_node(sw)
        network.add_link(Link(sw.name, 'eth0', 'gw',
                              'eth%d' % (subnet + 1 + subnets)))
        for i in range(1, hosts + 1):
            host = Node('private%d-host%d' % (subnet, i))
            second_last_octet = ((i + 1) // 256) % 256
            last_octet = (i + 1) % 256
            host.add_interface(Interface('eth0',
                '11.%d.%d.%d/16' % (subnet, second_last_octet, last_octet)))
            host.add_static_route(Route('0.0.0.0/0', '11.%d.0.1' % subnet))
            network.add_node(host)
            network.add_link(Link(host.name, 'eth0', sw.name, 'eth%d' % i))

    ## add nodes and links in the quarantined subnets
    for subnet in range(subnets):
        sw = Node('quarantined%d-sw' % subnet)
        sw.add_interface(Interface('eth0'))
        for i in range(1, hosts + 1):
            sw.add_interface(Interface('eth%d' % i))
        network.add_node(sw)
        network.add_link(Link(sw.name, 'eth0', 'gw',
                              'eth%d' % (subnet + 1 + subnets * 2)))
        for i in range(1, hosts + 1):
            host = Node('quarantined%d-host%d' % (subnet, i))
            second_last_octet = ((i + 1) // 256) % 256
            last_octet = (i + 1) % 256
            host.add_interface(Interface('eth0',
                '10.%d.%d.%d/16' % (subnet, second_last_octet, last_octet)))
            host.add_static_route(Route('0.0.0.0/0', '10.%d.0.1' % subnet))
            network.add_node(host)
            network.add_link(Link(host.name, 'eth0', sw.name, 'eth%d' % i))

    ## add policies
    # public subnets can initiate connections to the outside world
    policies.add_policy(ReachabilityPolicy(
        protocol = 'http',
        pkt_dst = '8.0.0.1',
        start_node = 'public[0-9]+-host[0-9]+',
        final_node = 'internet',
        reachable = True))
    # public subnets can accept connections from the outside world
    policies.add_policy(ReachabilityPolicy(
        protocol = 'http',
        pkt_dst = '12.0.0.0/8',
        owned_dst_only = True,
        start_node = 'internet',
        final_node = '(public[0-9]+-host[0-9]+)|gw',
        reachable = True))
    # private subnets can initiate connections to the outside world
    policies.add_policy(ReachabilityPolicy(
        protocol = 'http',
        pkt_dst = '8.0.0.1',
        start_node = 'private[0-9]+-host[0-9]+',
        final_node = 'internet',
        reachable = True))
    # replies from the outside world can reach the private subnets
    policies.add_policy(ReplyReachabilityPolicy(
        protocol = 'http',
        pkt_dst = '8.0.0.1',
        start_node = 'private[0-9]+-host[0-9]+',
        query_node = 'internet',
        reachable = True))
    # private subnets can't accept connections from the outside world
    policies.add_policy(ReachabilityPolicy(
        protocol = 'http',
        pkt_dst = '11.0.0.0/8',
        start_node = 'internet',
        final_node = 'private[0-9]+-host[0-9]+',
        reachable = False))
    # quarantined subnets can't initiate connections to the outside world
    policies.add_policy(ReachabilityPolicy(
        protocol = 'http',
        pkt_dst = '8.0.0.1',
        start_node = 'quarantined[0-9]+-host[0-9]+',
        final_node = 'internet',
        reachable = False))
    # quarantined subnets can't accept connections from the outside world
    policies.add_policy(ReachabilityPolicy(
        protocol = 'http',
        pkt_dst = '10.0.0.0/8',
        start_node = 'internet',
        final_node = 'quarantined[0-9]+-host[0-9]+',
        reachable = False))

    ## output as TOML
    config = network.to_dict()
    config.update(policies.to_dict())
    print(toml.dumps(config))

def main():
    parser = argparse.ArgumentParser(description='01-subnet-isolation')
    parser.add_argument('-s', '--subnets', type=int,
                        help='Number of subnets in each policy zone')
    parser.add_argument('-H', '--hosts', type=int,
                        help='Number of hosts in each subnet')
    arg = parser.parse_args()

    if not arg.subnets or arg.subnets > 256:
        sys.exit('Invalid number of subnets: ' + str(arg.subnets))
    if not arg.hosts or arg.hosts > 65533:
        sys.exit('Invalid number of hosts: ' + str(arg.hosts))

    confgen(arg.subnets, arg.hosts)

if __name__ == '__main__':
    main()
