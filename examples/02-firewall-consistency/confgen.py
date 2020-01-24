#!/usr/bin/python3

import sys
import toml
import argparse
from conf_classes import *

def confgen(apps, hosts):
    network = Network()
    policies = Policies()

    ## set firewall rules first
    fw_rules = """
*filter
:INPUT DROP [0:0]
:FORWARD DROP [0:0]
:OUTPUT ACCEPT [0:0]
"""
    for app in range(apps):
        second_octet = (app // 256) % 256
        third_octet = app % 256
        fw_rules += ('-A FORWARD -i eth0 -s 10.%d.%d.0/24 -d 11.%d.%d.0/24 -j ACCEPT\n'
                % (second_octet, third_octet, second_octet, third_octet))
        fw_rules += ('-A FORWARD -i eth0 -s 11.%d.%d.0/24 -d 10.%d.%d.0/24 -j ACCEPT\n'
                % (second_octet, third_octet, second_octet, third_octet))
    fw_rules += 'COMMIT\n'

    ## add the core, aggregation, and firewall nodes/links
    core1 = Node('core1')
    core1.add_interface(Interface('eth0', '9.0.0.1/24'))
    core1.add_interface(Interface('eth1', '9.0.1.1/24'))
    core1.add_static_route(Route('10.0.0.0/7', '9.0.0.2'))
    agg1_sink = Node('agg1-sink')
    agg1_sink.add_interface(Interface('eth0', '9.0.2.1/24'))
    agg1_sink.add_interface(Interface('eth1', '9.0.0.2/24'))
    agg1_sink.add_interface(Interface('eth2', '9.0.8.1/24'))
    agg1_sink.add_interface(Interface('eth3', '9.0.9.1/24'))
    agg1_sink.add_static_route(Route('0.0.0.0/0', '9.0.2.2'))
    fw1 = Middlebox('fw1', 'netns', 'netfilter')
    fw1.add_interface(Interface('eth0', '9.0.2.2/24'))
    fw1.add_interface(Interface('eth1', '9.0.3.1/24'))
    fw1.add_static_route(Route('0.0.0.0/0', '9.0.3.2'))
    fw1.add_config('rp_filter', 0)
    fw1.add_config('rules', fw_rules)
    agg1_source = Node('agg1-source')
    agg1_source.add_interface(Interface('eth0', '9.0.3.2/24'))
    agg1_source.add_interface(Interface('eth1', '9.0.1.2/24'))
    agg1_source.add_interface(Interface('eth2', '9.0.10.1/24'))
    agg1_source.add_interface(Interface('eth3', '9.0.11.1/24'))
    agg1_source.add_static_route(Route('10.0.0.0/8', '9.0.10.2'))
    agg1_source.add_static_route(Route('11.0.0.0/8', '9.0.11.2'))
    agg1_source.add_static_route(Route('0.0.0.0/0', '9.0.1.1'))
    core2 = Node('core2')
    core2.add_interface(Interface('eth0', '9.0.4.1/24'))
    core2.add_interface(Interface('eth1', '9.0.5.1/24'))
    core2.add_static_route(Route('10.0.0.0/7', '9.0.4.2'))
    agg2_sink = Node('agg2-sink')
    agg2_sink.add_interface(Interface('eth0', '9.0.6.1/24'))
    agg2_sink.add_interface(Interface('eth1', '9.0.4.2/24'))
    agg2_sink.add_interface(Interface('eth2', '9.0.12.1/24'))
    agg2_sink.add_interface(Interface('eth3', '9.0.13.1/24'))
    agg2_sink.add_static_route(Route('0.0.0.0/0', '9.0.6.2'))
    fw2 = Middlebox('fw2', 'netns', 'netfilter')
    fw2.add_interface(Interface('eth0', '9.0.6.2/24'))
    fw2.add_interface(Interface('eth1', '9.0.7.1/24'))
    fw2.add_static_route(Route('0.0.0.0/0', '9.0.7.2'))
    fw2.add_config('rp_filter', 0)
    fw2.add_config('rules', fw_rules)
    agg2_source = Node('agg2-source')
    agg2_source.add_interface(Interface('eth0', '9.0.7.2/24'))
    agg2_source.add_interface(Interface('eth1', '9.0.5.2/24'))
    agg2_source.add_interface(Interface('eth2', '9.0.14.1/24'))
    agg2_source.add_interface(Interface('eth3', '9.0.15.1/24'))
    agg2_source.add_static_route(Route('10.0.0.0/8', '9.0.14.2'))
    agg2_source.add_static_route(Route('11.0.0.0/8', '9.0.15.2'))
    agg2_source.add_static_route(Route('0.0.0.0/0', '9.0.5.1'))
    network.add_node(core1)
    network.add_node(agg1_sink)
    network.add_node(fw1)
    network.add_node(agg1_source)
    network.add_node(core2)
    network.add_node(agg2_sink)
    network.add_node(fw2)
    network.add_node(agg2_source)
    network.add_link(Link('core1', 'eth0', 'agg1-sink', 'eth1'))
    network.add_link(Link('core1', 'eth1', 'agg1-source', 'eth1'))
    network.add_link(Link('agg1-sink', 'eth0', 'fw1', 'eth0'))
    network.add_link(Link('agg1-source', 'eth0', 'fw1', 'eth1'))
    network.add_link(Link('core2', 'eth0', 'agg2-sink', 'eth1'))
    network.add_link(Link('core2', 'eth1', 'agg2-source', 'eth1'))
    network.add_link(Link('agg2-sink', 'eth0', 'fw2', 'eth0'))
    network.add_link(Link('agg2-source', 'eth0', 'fw2', 'eth1'))

    ## add access nodes/links
    access1 = Node('access1')
    access1.add_interface(Interface('eth0', '9.0.8.2/24'))
    access1.add_interface(Interface('eth1', '9.0.10.2/24'))
    access1.add_interface(Interface('eth2', '9.0.12.2/24'))
    access1.add_interface(Interface('eth3', '9.0.14.2/24'))
    access1.add_static_route(Route('0.0.0.0/0', '9.0.8.1'))
    access1.add_static_route(Route('0.0.0.0/0', '9.0.12.1'))
    access2 = Node('access2')
    access2.add_interface(Interface('eth0', '9.0.9.2/24'))
    access2.add_interface(Interface('eth1', '9.0.11.2/24'))
    access2.add_interface(Interface('eth2', '9.0.13.2/24'))
    access2.add_interface(Interface('eth3', '9.0.15.2/24'))
    access2.add_static_route(Route('0.0.0.0/0', '9.0.9.1'))
    access2.add_static_route(Route('0.0.0.0/0', '9.0.13.1'))
    network.add_node(access1)
    network.add_node(access2)
    network.add_link(Link('access1', 'eth0', 'agg1-sink',   'eth2'))
    network.add_link(Link('access1', 'eth1', 'agg1-source', 'eth2'))
    network.add_link(Link('access1', 'eth2', 'agg2-sink',   'eth2'))
    network.add_link(Link('access1', 'eth3', 'agg2-source', 'eth2'))
    network.add_link(Link('access2', 'eth0', 'agg1-sink',   'eth3'))
    network.add_link(Link('access2', 'eth1', 'agg1-source', 'eth3'))
    network.add_link(Link('access2', 'eth2', 'agg2-sink',   'eth3'))
    network.add_link(Link('access2', 'eth3', 'agg2-source', 'eth3'))

    ## add application hosts and related nodes/links
    for app in range(apps):
        for host in range(hosts):
            node = Node('app%d-host%d' % (app, host))
            second = (app // 256) % 256
            third = app % 256
            last = 4 * (host // 2) + 2
            acc_intf_num = 64 * app + host // 2 + 4
            if host % 2 == 0:   # hosts under access1
                access1.add_interface(Interface('eth%d' % acc_intf_num,
                    '10.%d.%d.%d/30' % (second, third, last - 1)))
                node.add_interface(Interface('eth0',
                    '10.%d.%d.%d/30' % (second, third, last)))
                node.add_static_route(Route('0.0.0.0/0',
                    '10.%d.%d.%d' % (second, third, last - 1)))
                network.add_link(Link(node.name, 'eth0', access1.name,
                    'eth%d' % acc_intf_num))
            elif host % 2 == 1: # hosts under access2
                access2.add_interface(Interface('eth%d' % acc_intf_num,
                    '11.%d.%d.%d/30' % (second, third, last - 1)))
                node.add_interface(Interface('eth0',
                    '11.%d.%d.%d/30' % (second, third, last)))
                node.add_static_route(Route('0.0.0.0/0',
                    '11.%d.%d.%d' % (second, third, last - 1)))
                network.add_link(Link(node.name, 'eth0', access2.name,
                    'eth%d' % acc_intf_num))
            network.add_node(node)

    ## add policies
    ## TODO: change them to consistency policies of reachability
    for app in range(apps):
        second = (app // 256) % 256 # second octet
        third = app % 256           # third octet
        hosts_acc1 = 'app%d-host(' % app
        hosts_acc2 = 'app%d-host(' % app
        for host in range(hosts):
            if host % 2 == 0:
                if host != 0:
                    hosts_acc1 += '|'
                hosts_acc1 += str(host)
            elif host % 2 == 1:
                if host != 1:
                    hosts_acc2 += '|'
                hosts_acc2 += str(host)
        hosts_acc1 += ')'
        hosts_acc2 += ')'
        other_apps = list(range(apps))
        other_apps.remove(app)
        hosts_other_apps = ''
        if other_apps:
            hosts_other_apps = ('app(%s)-host[0-9]+' %
                    ('|'.join([str(i) for i in other_apps])))
        # In the same application, hosts under access1 can reach hosts under
        # access2
        policies.add_policy(ReachabilityPolicy(
            protocol = 'http',
            pkt_dst = '11.%d.%d.0/24' % (second, third),
            owned_dst_only = True,
            start_node = hosts_acc1,
            final_node = '(' + hosts_acc2 + ')|access2',
            reachable = True))
        # In the same application, hosts under access2 can reach hosts under
        # access1
        policies.add_policy(ReachabilityPolicy(
            protocol = 'http',
            pkt_dst = '10.%d.%d.0/24' % (second, third),
            owned_dst_only = True,
            start_node = hosts_acc2,
            final_node = '(' + hosts_acc1 + ')|access1',
            reachable = True))
        # Hosts under an application cannot reach hosts under other applications
        policies.add_policy(ReachabilityPolicy(
            protocol = 'http',
            pkt_dst = '10.0.0.0/7',
            owned_dst_only = True,
            start_node = 'app%d-host[0-9]+' % app,
            final_node = hosts_other_apps,
            reachable = False))

    ## output as TOML
    config = network.to_dict()
    config.update(policies.to_dict())
    print(toml.dumps(config))

def main():
    parser = argparse.ArgumentParser(description='02-firewall-consistency')
    parser.add_argument('-a', '--apps', type=int,
                        help='Number of applications')
    parser.add_argument('-H', '--hosts', type=int,
                        help='Number of hosts in each application')
    arg = parser.parse_args()

    if not arg.apps or arg.apps > 65536:
        sys.exit('Invalid number of subnets: ' + str(arg.subnets))
    if not arg.hosts or arg.hosts > 128:
        sys.exit('Invalid number of hosts: ' + str(arg.hosts))

    confgen(arg.apps, arg.hosts)

if __name__ == '__main__':
    main()
