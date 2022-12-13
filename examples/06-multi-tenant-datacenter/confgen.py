#!/usr/bin/python3

import os
import sys
import argparse

sys.path.append(os.path.join(os.path.dirname(__file__), '../../src'))
from config import *


def confgen(tenants, updates, servers):
    config = Config()

    ## firewall rules
    fw_rules = """
*filter
:INPUT DROP [0:0]
:FORWARD DROP [0:0]
:OUTPUT ACCEPT [0:0]
-A FORWARD -i eth0 -d 10.0.0.0/8 -m conntrack --ctstate RELATED,ESTABLISHED -j ACCEPT
-A FORWARD -i eth1 -s 10.0.0.0/8 -j ACCEPT
COMMIT
"""

    ## add the Internet node, gateway, and central switch
    internet = Node('internet')
    internet.add_interface(Interface('eth0', '8.1.0.2/16'))
    internet.add_static_route(Route('8.0.0.0/8', '8.1.0.1'))
    internet.add_static_route(Route('9.0.0.0/8', '8.1.0.1'))
    internet.add_static_route(Route('10.0.0.0/8', '8.1.0.1'))
    config.add_node(internet)
    gateway = Node('gateway')
    gateway.add_interface(Interface('eth0', '8.0.0.1/16'))
    gateway.add_interface(Interface('eth1', '8.1.0.1/16'))
    config.add_node(gateway)
    switch = Node('sw')
    switch.add_interface(Interface('eth0'))
    config.add_node(switch)
    config.add_link(Link('internet', 'eth0', 'gateway', 'eth1'))
    config.add_link(Link('sw', 'eth0', 'gateway', 'eth0'))

    ## add tenant networks
    for tenant_id in range(tenants):
        X = (tenant_id // 256) % 256
        Y = tenant_id % 256
        X_2 = ((tenant_id + 2) // 256) % 256
        Y_2 = (tenant_id + 2) % 256

        ## add routers and firewall
        r1 = Node('r%d.1' % tenant_id)
        r2 = Node('r%d.2' % tenant_id)
        fw = Middlebox('fw%d' % tenant_id, 'netns', 'netfilter')
        r1.add_interface(Interface('eth0', '8.0.%d.%d/16' % (X_2, Y_2)))
        r1.add_interface(Interface('eth1', '9.%d.%d.1/30' % (X, Y)))
        r1.add_interface(Interface('eth2', '9.%d.%d.5/30' % (X, Y)))
        r1.add_static_route(
            Route('10.%d.%d.0/24' % (X, Y), '9.%d.%d.6' % (X, Y)))
        r1.add_static_route(Route('0.0.0.0/0', '8.0.0.1'))
        r2.add_interface(Interface('eth0', '9.%d.%d.2/30' % (X, Y)))
        r2.add_interface(Interface('eth1', '9.%d.%d.10/30' % (X, Y)))
        r2.add_interface(Interface('eth2', '10.%d.%d.1/24' % (X, Y)))
        r2.add_static_route(Route('0.0.0.0/0', '9.%d.%d.9' % (X, Y)))
        fw.add_interface(Interface('eth0', '9.%d.%d.6/30' % (X, Y)))
        fw.add_interface(Interface('eth1', '9.%d.%d.9/30' % (X, Y)))
        fw.add_static_route(
            Route('10.%d.%d.0/24' % (X, Y), '9.%d.%d.10' % (X, Y)))
        fw.add_static_route(Route('0.0.0.0/0', '9.%d.%d.5' % (X, Y)))
        fw.add_config('rp_filter', 0)
        fw.add_config('rules', fw_rules)
        config.add_node(r1)
        config.add_node(r2)
        config.add_node(fw)
        config.add_link(Link(r1.name, 'eth1', r2.name, 'eth0'))
        config.add_link(Link(r1.name, 'eth2', fw.name, 'eth0'))
        config.add_link(Link(r2.name, 'eth1', fw.name, 'eth1'))

        ## connect to the central switch and add routes at the gateway
        switch.add_interface(Interface('eth%d' % (tenant_id + 1)))
        config.add_link(
            Link(switch.name, 'eth%d' % (tenant_id + 1), r1.name, 'eth0'))
        gateway.add_static_route(
            Route('10.%d.%d.0/24' % (X, Y), '8.0.%d.%d' % (X_2, Y_2)))

        ## add private servers connecting to r2
        tenant_sw = Node('sw%d' % tenant_id)
        tenant_sw.add_interface(Interface('eth0'))
        config.add_node(tenant_sw)
        config.add_link(Link(tenant_sw.name, 'eth0', r2.name, 'eth2'))
        for server_id in range(servers):
            server = Node('server%d.%d' % (tenant_id, server_id))
            server.add_interface(
                Interface('eth0', '10.%d.%d.%d/24' % (X, Y, server_id + 2)))
            server.add_static_route(Route('0.0.0.0/0', '10.%d.%d.1' % (X, Y)))
            config.add_node(server)
            tenant_sw.add_interface(Interface('eth%d' % (server_id + 1)))
            config.add_link(
                Link(server.name, 'eth0', tenant_sw.name,
                     'eth%d' % (server_id + 1)))

    ## add policies
    # private servers can initiate connections to the outside world and the
    # replies from the outside world can reach the private subnets
    config.add_policy(
        ReplyReachabilityPolicy(target_node='internet',
                                reachable=True,
                                protocol='tcp',
                                src_node='server.*\..*',
                                dst_ip='8.1.0.2',
                                dst_port=80,
                                owned_dst_only=True))
    # private servers can't accept connections from the outside world
    config.add_policy(
        ReachabilityPolicy(target_node='(server.*\..*)|r.*\.2',
                           reachable=False,
                           protocol='tcp',
                           src_node='internet',
                           dst_ip='10.0.0.0/8',
                           dst_port=80,
                           owned_dst_only=True))

    ## add route updates
    for update in range(updates):
        # Note that the sampling of tenants that get updated should not affect
        # the overall performance.
        tenant_id = update
        X = (tenant_id // 256) % 256
        Y = tenant_id % 256
        r1_name = 'r%d.1' % tenant_id
        r2_name = 'r%d.2' % tenant_id
        config.add_update(
            OpenflowUpdate(r1_name, '10.%d.%d.0/24' % (X, Y), 'eth1'))
        config.add_update(OpenflowUpdate(r2_name, '0.0.0.0/0', 'eth0'))

    ## output as TOML
    config.output_toml()


def main():
    parser = argparse.ArgumentParser(description='06-multi-tenant-datacenter')
    parser.add_argument('-t', '--tenants', type=int, help='Number of tenants')
    parser.add_argument('-u',
                        '--updates',
                        type=int,
                        help='Number of tenants that get updated')
    parser.add_argument('-s',
                        '--servers',
                        type=int,
                        help='Number of servers within each tenant')
    arg = parser.parse_args()

    if not arg.tenants or arg.tenants > 65533:
        sys.exit('Invalid number of tenants: ' + str(arg.tenants))
    if arg.updates is None or arg.updates > arg.tenants:
        sys.exit('Invalid number of updates: ' + str(arg.updates))
    if not arg.servers or arg.servers > 253:
        sys.exit('Invalid number of servers per tenant: ' + str(arg.servers))

    confgen(arg.tenants, arg.updates, arg.servers)


if __name__ == '__main__':
    main()
