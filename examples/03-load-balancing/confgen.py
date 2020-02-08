#!/usr/bin/python3

import sys
import toml
import argparse
from conf_classes import *

def confgen(lbs, servers, algorithm, additional_repeat):
    network = Network()
    policies = Policies()

    ## add the Internet node and r1
    internet_node = Node('internet')
    internet_node.add_interface(Interface('eth0', '8.0.0.2/24'))
    internet_node.add_static_route(Route('0.0.0.0/0', '8.0.0.1'))
    r1 = Node('r1')
    r1.add_interface(Interface('eth0', '8.0.0.1/24'))
    for lb in range(1, lbs + 1):
        r1.add_interface(Interface('eth%d' % lb, '8.0.%d.1/24' % lb))
        r1.add_static_route(Route('9.%d.0.0/16' % lb, '8.0.%d.2' % lb))
    network.add_node(internet_node)
    network.add_node(r1)
    network.add_link(Link('internet', 'eth0', 'r1', 'eth0'))

    ## add the load balancers, switches, and servers
    for lb in range(1, lbs + 1):
        load_balancer = Middlebox('lb%d' % lb, 'netns', 'ipvs')
        load_balancer.add_interface(Interface('eth0', '8.0.%d.2/24' % lb))
        load_balancer.add_interface(Interface('eth1', '9.%d.0.1/16' % lb))
        load_balancer.add_static_route(Route('0.0.0.0/0', '8.0.%d.1' % lb))
        load_balancer.set_timeout(100)
        lb_config = ""
        lb_config += "-A -t 8.0.%d.2:80 -s %s\n" % (lb, algorithm)
        sw = Node('sw%d' % lb)
        sw.add_interface(Interface('eth0'))
        network.add_node(load_balancer)
        network.add_node(sw)
        network.add_link(Link('r1', 'eth%d' % lb, load_balancer.name, 'eth0'))
        network.add_link(Link(load_balancer.name, 'eth1', sw.name, 'eth0'))
        for srv in range(1, servers + 1):
            sw.add_interface(Interface('eth%d' % srv))
            server = Node('server%d.%d' % (lb, srv))
            third = ((srv + 1) // 256) % 256
            last = (srv + 1) % 256
            server.add_interface(Interface('eth0', '9.%d.%d.%d/24' % (lb, third, last)))
            server.add_static_route(Route('0.0.0.0/0', '9.%d.0.1' % lb))
            lb_config += "-a -t 8.0.%d.2:80 -r 9.%d.%d.%d:80 -m\n" % (lb, lb, third, last)
            network.add_node(server)
            network.add_link(Link(sw.name, 'eth%d' % srv, server.name, 'eth0'))
        load_balancer.add_config('config', lb_config)

    ## add policies
    for lb in range(1, lbs + 1):
        policies.add_policy(LoadBalancePolicy(
                protocol = 'http',
                pkt_dst = '8.0.%d.2' % lb,
                start_node = internet_node.name,
                final_node = 'server%d\.[0-9]+' % lb,
                repeat = servers + additional_repeat))

    ## output as TOML
    config = network.to_dict()
    config.update(policies.to_dict())
    print(toml.dumps(config))

def main():
    parser = argparse.ArgumentParser(description='02-firewall-consistency')
    parser.add_argument('-l', '--lbs', type=int,
                        help='Number of load balancers (HTTP servers)')
    parser.add_argument('-s', '--servers', type=int,
                        help='Number of servers behind each LB')
    parser.add_argument('-a', '--algorithm', choices=['rr', 'sh', 'dh'],
                        help='Load balancing algorithm')
    parser.add_argument('-r', '--repeat', type=int,
                        help='Repeat N more times than the number of servers')
    #parser.add_argument('-f', '--fault', action='store_true', default=False,
    #                    help='Use inconsistent rules')
    arg = parser.parse_args()

    if not arg.lbs or arg.lbs > 255:
        sys.exit('Invalid number of load balancers: ' + str(arg.lbs))
    if not arg.servers or arg.servers > 65533:
        sys.exit('Invalid number of servers: ' + str(arg.servers))
    if not arg.repeat:
        arg.repeat = 0

    confgen(arg.lbs, arg.servers, arg.algorithm, arg.repeat)

if __name__ == '__main__':
    main()
