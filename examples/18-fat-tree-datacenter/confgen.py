import os
import sys
import argparse

sys.path.append(os.path.join(os.path.dirname(__file__), '../../src'))
from config import *

def confgen(k, servers, model_only=False):
    config = Config()

    fw_rules = """
    *filter
    :INPUT DROP [0:0]
    :FORWARD DROP [0:0]
    :OUTPUT ACCEPT [0:0]
    -A FORWARD -i eth0 -d 10.0.0.0/8 -m conntrack --ctstate RELATED,ESTABLISHED -j ACCEPT
    -A FORWARD -i eth1 -s 10.0.0.0/8 -j ACCEPT
    COMMIT
    """
    #internet and gateway
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

    #add fat tree switches
    for s in range(1, 1 + (k // 2) ** 2 + k ** 2):
        switch = Node('sw' + str(s))
        for i in range(k):
            switch.add_interface(Interface('eth'+str(i)))
        config.add_node(switch)
    index = 1
    core_switches = range(index, index + (k // 2) ** 2)
    index += len(core_switches)
    aggr_switches = range(index, index + k ** 2 // 2)
    index += len(aggr_switches)
    edge_switches = range(index, index + k ** 2 // 2)
    index += len(edge_switches)
    del index

    #add links for internet, gataway, switches

    config.add_link(Link('internet', 'eth0', 'gateway', 'eth1'))
    config.add_link(Link('sw'+str(edge_switches[0]), 'eth2', 'gateway', 'eth0'))

    #add tenant networks
    tenants=range(1, k**3 // 4)
    for tenant_id in tenants:
        X = tenant_id // 256
        Y = tenant_id % 256
        X_2 = ((tenant_id + 2) // 256)
        Y_2 = (tenant_id + 2) % 256
        if X > 255:
            raise Exception("tenantgen:too many tenants")
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
        fw.add_interface(Interface('eth0', '9.%d.%d.6/30' % (X, Y)))
        fw.add_interface(Interface('eth1', '9.%d.%d.9/30' % (X, Y)))
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

        ## connect to the edge switch and add routes at the gateway

        gateway.add_static_route(Route('10.%d.%d.0/24' % (X, Y), '8.0.%d.%d' % (X_2, Y_2)))

        ## add private servers connecting to r2
        tenant_sw = Node('t%dsw' % tenant_id)
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


    ## Core-Aggr links
    for i, core in enumerate(core_switches):
        for j in range(0, k):
            aggr = aggr_switches[j * (k // 2) + i // (k // 2)]
            config.add_link(Link('sw'+str(core), 'eth'+str(j), 'sw'+str(aggr), 'eth'+str(i % (k // 2))))

    ## Aggr-Edge links
    for i, aggr in enumerate(aggr_switches):
        for j in range(0, k // 2):
            edge = edge_switches[(i - i % (k // 2)) + j]
            config.add_link(Link('sw'+str(aggr), 'eth'+str(j + k // 2), 'sw'+str(edge), 'eth'+str(i % (k // 2))))

    ## Edge-Host links
    for i, edge in enumerate(edge_switches):
        for j in range(0, k // 2):
            host = i * (k // 2) + j
            if host==0:
                continue
            config.add_link(Link('sw'+str(edge), 'eth'+str(j + k // 2), 'r'+str(host)+'.1', 'eth0'))

    ## add invariants
    # private servers can initiate connections to the outside world and the
    # replies from the outside world can reach the private subnets
    config.add_invariant(
        ReplyReachability(target_node='internet',
                          reachable=True,
                          protocol='tcp',
                          src_node='server.*\..*',
                          dst_ip='8.1.0.2',
                          dst_port=80,
                          owned_dst_only=True))
    # private servers can't accept connections from the outside world
    config.add_invariant(
        Reachability(target_node='(server.*\..*)|r.*\.2',
                     reachable=False,
                     protocol='tcp',
                     src_node='internet',
                     dst_ip='10.0.0.0/8',
                     dst_port=80,
                     owned_dst_only=True))

    config.output_toml()
def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('-k',
                        '--arity',
                        type=int,
                        help='Fat tree arity',
                        required=True)
    parser.add_argument('-s',
                        '--servers',
                        type=int,
                        help='Number of servers within each tenant')
    arg = parser.parse_args()

    if not arg.servers or arg.servers > 253:
        sys.exit('Invalid number of servers per tenant: ' + str(arg.servers))
    confgen(arg.arity, arg.servers)


if __name__ == '__main__':
    main()
