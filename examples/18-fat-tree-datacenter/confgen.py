#!/usr/bin/python3

import os
import sys
import argparse

sys.path.append(os.path.join(os.path.dirname(__file__), '../../src'))
from config import *


def confgen(k, update_percentage, servers, model_only=False):
    config = Config()

    # Add fat-tree switches
    for s in range(1, 1 + (k // 2)**2 + k**2):
        switch = Node('sw{}'.format(s))
        for i in range(k):
            switch.add_interface(Interface('eth{}'.format(i)))
        config.add_node(switch)
    index = 1
    core_switches = range(index, index + (k // 2)**2)
    index += len(core_switches)
    aggr_switches = range(index, index + k**2 // 2)
    index += len(aggr_switches)
    edge_switches = range(index, index + k**2 // 2)
    index += len(edge_switches)
    del index

    # Add fat-tree link
    ## Core-Aggr links
    for i, core in enumerate(core_switches):
        for j in range(0, k):
            aggr = aggr_switches[j * (k // 2) + i // (k // 2)]
            config.add_link(
                Link('sw{}'.format(core), 'eth{}'.format(j),
                     'sw{}'.format(aggr), 'eth{}'.format(i % (k // 2))))

    ## Aggr-Edge links
    for i, aggr in enumerate(aggr_switches):
        for j in range(0, k // 2):
            edge = edge_switches[(i - i % (k // 2)) + j]
            config.add_link(
                Link('sw{}'.format(aggr), 'eth{}'.format(j + k // 2),
                     'sw{}'.format(edge), 'eth{}'.format(i % (k // 2))))

    ## Edge-Host links
    for i, edge in enumerate(edge_switches):
        for j in range(0, k // 2):
            host = i * (k // 2) + j
            if host == 0:
                # Connected to the gateway and Internet node.
                # i == 0 and j == 0
                config.add_link(
                    Link('sw{}'.format(edge), 'eth{}'.format(j + k // 2),
                         'gateway', 'eth0'))
            else:
                config.add_link(
                    Link('sw{}'.format(edge), 'eth{}'.format(j + k // 2),
                         'r{}.1'.format(host), 'eth0'))

    # Add the internet and gateway nodes
    gateway = Node('gateway')
    gateway.add_interface(Interface('eth0', '8.0.0.1/16'))  # internal
    gateway.add_interface(Interface('eth1', '7.0.0.1/16'))  # external
    config.add_node(gateway)
    internet = Node('internet')
    internet.add_interface(Interface('eth0', '7.0.0.2/16'))
    internet.add_static_route(Route('0.0.0.0/0', '7.0.0.1'))
    config.add_node(internet)

    # Add links between the internet and gataway nodes
    config.add_link(Link('gateway', 'eth1', 'internet', 'eth0'))

    fw_rules = """
*filter
:INPUT DROP [0:0]
:FORWARD DROP [0:0]
:OUTPUT ACCEPT [0:0]
-A FORWARD -i eth0 -d 10.0.0.0/8 -m conntrack --ctstate RELATED,ESTABLISHED -j ACCEPT
-A FORWARD -i eth1 -s 10.0.0.0/8 -j ACCEPT
COMMIT
"""

    # Add tenant networks
    for tenant_id in range(1, k**3 // 4):
        X = tenant_id // 256
        Y = tenant_id % 256
        X_2 = ((tenant_id + 1) // 256)
        Y_2 = (tenant_id + 1) % 256
        if X > 255 or X_2 > 255:  # k max: 63
            raise Exception("Too many tenants")

        r1 = Node('r%d.1' % tenant_id)
        r2 = Node('r%d.2' % tenant_id)
        if model_only:
            fw = Node('fw%d' % tenant_id)
        else:
            fw = DockerNode('fw%d' % tenant_id,
                            image='kyechou/iptables:latest',
                            working_dir='/',
                            command=['/start.sh'])
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
        fw.add_interface(Interface('eth0',
                                   '9.%d.%d.6/30' % (X, Y)))  # external
        fw.add_interface(Interface('eth1',
                                   '9.%d.%d.9/30' % (X, Y)))  # internal
        fw.add_static_route(
            Route('10.%d.%d.0/24' % (X, Y), '9.%d.%d.10' % (X, Y)))
        fw.add_static_route(Route('0.0.0.0/0', '9.%d.%d.5' % (X, Y)))
        if model_only:
            # TODO
            pass
        else:
            fw.add_sysctl('net.ipv4.conf.all.forwarding', '1')
            fw.add_sysctl('net.ipv4.conf.all.rp_filter', '0')
            fw.add_sysctl('net.ipv4.conf.default.rp_filter', '0')
            fw.add_env_var('RULES', fw_rules)
        config.add_node(r1)
        config.add_node(r2)
        config.add_node(fw)
        config.add_link(Link(r1.name, 'eth1', r2.name, 'eth0'))
        config.add_link(Link(r1.name, 'eth2', fw.name, 'eth0'))
        config.add_link(Link(r2.name, 'eth1', fw.name, 'eth1'))

        ## Add routes to the tenant from the gateway
        gateway.add_static_route(
            Route('10.%d.%d.0/24' % (X, Y), '8.0.%d.%d' % (X_2, Y_2)))

        ## Add private servers connecting to r2
        tenant_sw = Node('sw-tenant-%d' % tenant_id)
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

    ## Add invariants
    # private servers can initiate connections to the outside world and the
    # replies from the outside world can reach the private subnets
    config.add_invariant(
        ReplyReachability(target_node='internet',
                          reachable=True,
                          protocol='icmp-echo',
                          src_node='server.*\..*',
                          dst_ip='7.0.0.2',
                          owned_dst_only=True))
    # private servers can't accept connections from the outside world
    config.add_invariant(
        Reachability(target_node='(server.*\..*)|r.*\.2',
                     reachable=False,
                     protocol='icmp-echo',
                     src_node='internet',
                     dst_ip='10.0.0.0/8',
                     owned_dst_only=True))

    # Add route updates
    num_tenants = k**3 // 4 - 1  # 0-th host is used as a gateway router
    num_updates = num_tenants * update_percentage // 100
    for update in range(num_updates):
        # Note that the sampling of tenants that get updated should not affect
        # the overall performance.
        tenant_id = update + 1  # "Tenant 0" is used as a gateway router
        X = tenant_id // 256
        Y = tenant_id % 256
        r1_name = 'r%d.1' % tenant_id
        r2_name = 'r%d.2' % tenant_id
        config.add_update(
            OpenflowUpdate(r1_name, '10.%d.%d.0/24' % (X, Y), 'eth1'))
        config.add_update(OpenflowUpdate(r2_name, '0.0.0.0/0', 'eth0'))

    config.output_toml()


def main():
    parser = argparse.ArgumentParser(description='18-fat-tree-datacenter')
    parser.add_argument('-k',
                        '--arity',
                        type=int,
                        required=True,
                        help='Fat-tree arity')
    parser.add_argument('-u',
                        '--updates',
                        type=int,
                        help='Percentage of tenants that get updated [0..100]')
    parser.add_argument('-s',
                        '--servers',
                        type=int,
                        required=True,
                        help='Number of servers within each tenant')
    parser.add_argument('-m',
                        '--model',
                        action='store_true',
                        default=False,
                        help='Use only models without emulations')
    arg = parser.parse_args()

    if not arg.arity or arg.arity % 2 != 0 or arg.arity > 63:
        sys.exit('Invalid fat-tree arity: ' + str(arg.arity))
    if arg.updates is None or arg.updates < 0 or arg.updates > 100:
        sys.exit('Invalid percentage of updating tenants: ' + str(arg.updates))
    if not arg.servers or arg.servers < 1 or arg.servers > 253:
        sys.exit('Invalid number of servers per tenant: ' + str(arg.servers))

    confgen(arg.arity, arg.updates, arg.servers, arg.model)


if __name__ == '__main__':
    main()
