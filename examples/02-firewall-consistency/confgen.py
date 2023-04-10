#!/usr/bin/python3

import os
import sys
import argparse

sys.path.append(os.path.join(os.path.dirname(__file__), '../../src'))
from config import *


def confgen(apps, hosts, fault):
    config = Config()

    ## set firewall rules
    fw_rules = """
*filter
:INPUT DROP [0:0]
:FORWARD DROP [0:0]
:OUTPUT ACCEPT [0:0]
"""
    for app in range(apps):
        second = (app // 256) % 256
        third = app % 256
        fw_rules += (
            '-A FORWARD -i eth0 -s 10.%d.%d.0/24 -d 11.%d.%d.0/24 -j ACCEPT\n'
            % (second, third, second, third))
        fw_rules += (
            '-A FORWARD -i eth0 -s 11.%d.%d.0/24 -d 10.%d.%d.0/24 -j ACCEPT\n'
            % (second, third, second, third))
    fw_rules += 'COMMIT\n'

    ## misconfigured firewall rules
    wrong_fw_rules = """
*filter
:INPUT DROP [0:0]
:FORWARD ACCEPT [0:0]
:OUTPUT ACCEPT [0:0]
"""
    for app in range(apps):
        second = (app // 256) % 256
        third = app % 256
        wrong_fw_rules += (
            '-A FORWARD -i eth0 -s 10.%d.%d.0/24 -d 11.%d.%d.0/24 -j DROP\n' %
            (second, third, second, third))
        wrong_fw_rules += (
            '-A FORWARD -i eth0 -s 11.%d.%d.0/24 -d 10.%d.%d.0/24 -j DROP\n' %
            (second, third, second, third))
    wrong_fw_rules += 'COMMIT\n'

    ## add the core, aggregation, and firewall nodes/links
    core1 = Node('core1')
    core1.add_interface(Interface('eth0', '8.0.0.1/24'))
    core1.add_interface(Interface('eth1', '8.0.1.1/24'))
    agg1_sink = Node('agg1-sink')
    agg1_sink.add_interface(Interface('eth0', '8.0.2.1/24'))
    agg1_sink.add_interface(Interface('eth1', '8.0.0.2/24'))
    agg1_sink.add_static_route(Route('0.0.0.0/0', '8.0.2.2'))
    fw1 = DockerNode('fw1',
                     image='kyechou/iptables:latest',
                     working_dir='/',
                     command=['/start.sh'])
    fw1.add_interface(Interface('eth0', '8.0.2.2/24'))
    fw1.add_interface(Interface('eth1', '8.0.3.1/24'))
    fw1.add_static_route(Route('0.0.0.0/0', '8.0.3.2'))
    fw1.add_env_var('RULES', fw_rules)
    fw1.add_sysctl('net.ipv4.conf.all.forwarding', '1')
    fw1.add_sysctl('net.ipv4.conf.all.rp_filter', '0')
    fw1.add_sysctl('net.ipv4.conf.default.rp_filter', '0')
    agg1_source = Node('agg1-source')
    agg1_source.add_interface(Interface('eth0', '8.0.3.2/24'))
    agg1_source.add_interface(Interface('eth1', '8.0.1.2/24'))
    core2 = Node('core2')
    core2.add_interface(Interface('eth0', '8.0.4.1/24'))
    core2.add_interface(Interface('eth1', '8.0.5.1/24'))
    agg2_sink = Node('agg2-sink')
    agg2_sink.add_interface(Interface('eth0', '8.0.6.1/24'))
    agg2_sink.add_interface(Interface('eth1', '8.0.4.2/24'))
    agg2_sink.add_static_route(Route('0.0.0.0/0', '8.0.6.2'))
    fw2 = DockerNode('fw2',
                     image='kyechou/iptables:latest',
                     working_dir='/',
                     command=['/start.sh'])
    fw2.add_interface(Interface('eth0', '8.0.6.2/24'))
    fw2.add_interface(Interface('eth1', '8.0.7.1/24'))
    fw2.add_static_route(Route('0.0.0.0/0', '8.0.7.2'))
    fw2.add_env_var('RULES', fw_rules if not fault else wrong_fw_rules)
    fw2.add_sysctl('net.ipv4.conf.all.forwarding', '1')
    fw2.add_sysctl('net.ipv4.conf.all.rp_filter', '0')
    fw2.add_sysctl('net.ipv4.conf.default.rp_filter', '0')
    agg2_source = Node('agg2-source')
    agg2_source.add_interface(Interface('eth0', '8.0.7.2/24'))
    agg2_source.add_interface(Interface('eth1', '8.0.5.2/24'))
    config.add_node(core1)
    config.add_node(agg1_sink)
    config.add_node(fw1)
    config.add_node(agg1_source)
    config.add_node(core2)
    config.add_node(agg2_sink)
    config.add_node(fw2)
    config.add_node(agg2_source)
    config.add_link(Link('core1', 'eth0', 'agg1-sink', 'eth1'))
    config.add_link(Link('core1', 'eth1', 'agg1-source', 'eth1'))
    config.add_link(Link('agg1-sink', 'eth0', 'fw1', 'eth0'))
    config.add_link(Link('agg1-source', 'eth0', 'fw1', 'eth1'))
    config.add_link(Link('core2', 'eth0', 'agg2-sink', 'eth1'))
    config.add_link(Link('core2', 'eth1', 'agg2-source', 'eth1'))
    config.add_link(Link('agg2-sink', 'eth0', 'fw2', 'eth0'))
    config.add_link(Link('agg2-source', 'eth0', 'fw2', 'eth1'))

    for app in range(apps):
        ## add access nodes/links
        second = (app // 256) % 256
        third = app % 256
        agg1_sink.add_interface(
            Interface('eth%d' % (2 * app + 2),
                      '9.%d.%d.1/30' % (second, third)))
        agg1_sink.add_interface(
            Interface('eth%d' % (2 * app + 3),
                      '9.%d.%d.5/30' % (second, third)))
        agg1_source.add_interface(
            Interface('eth%d' % (2 * app + 2),
                      '9.%d.%d.9/30' % (second, third)))
        agg1_source.add_interface(
            Interface('eth%d' % (2 * app + 3),
                      '9.%d.%d.13/30' % (second, third)))
        agg1_source.add_static_route(
            Route('10.%d.%d.0/24' % (second, third),
                  '9.%d.%d.10' % (second, third)))
        agg1_source.add_static_route(
            Route('11.%d.%d.0/24' % (second, third),
                  '9.%d.%d.14' % (second, third)))
        agg2_sink.add_interface(
            Interface('eth%d' % (2 * app + 2),
                      '9.%d.%d.17/30' % (second, third)))
        agg2_sink.add_interface(
            Interface('eth%d' % (2 * app + 3),
                      '9.%d.%d.21/30' % (second, third)))
        agg2_source.add_interface(
            Interface('eth%d' % (2 * app + 2),
                      '9.%d.%d.25/30' % (second, third)))
        agg2_source.add_interface(
            Interface('eth%d' % (2 * app + 3),
                      '9.%d.%d.29/30' % (second, third)))
        agg2_source.add_static_route(
            Route('10.%d.%d.0/24' % (second, third),
                  '9.%d.%d.26' % (second, third)))
        agg2_source.add_static_route(
            Route('11.%d.%d.0/24' % (second, third),
                  '9.%d.%d.30' % (second, third)))
        access1 = Node('access1-app%d' % app)
        access1.add_interface(
            Interface('eth0', '9.%d.%d.2/30' % (second, third)))
        access1.add_interface(
            Interface('eth1', '9.%d.%d.10/30' % (second, third)))
        access1.add_interface(
            Interface('eth2', '9.%d.%d.18/30' % (second, third)))
        access1.add_interface(
            Interface('eth3', '9.%d.%d.26/30' % (second, third)))
        access1.add_static_route(
            Route('0.0.0.0/0', '9.%d.%d.1' % (second, third)))
        access1.add_static_route(
            Route('0.0.0.0/0', '9.%d.%d.17' % (second, third)))
        access2 = Node('access2-app%d' % app)
        access2.add_interface(
            Interface('eth0', '9.%d.%d.6/30' % (second, third)))
        access2.add_interface(
            Interface('eth1', '9.%d.%d.14/30' % (second, third)))
        access2.add_interface(
            Interface('eth2', '9.%d.%d.22/30' % (second, third)))
        access2.add_interface(
            Interface('eth3', '9.%d.%d.30/30' % (second, third)))
        access2.add_static_route(
            Route('0.0.0.0/0', '9.%d.%d.5' % (second, third)))
        access2.add_static_route(
            Route('0.0.0.0/0', '9.%d.%d.21' % (second, third)))
        config.add_node(access1)
        config.add_node(access2)
        config.add_link(
            Link(access1.name, 'eth0', 'agg1-sink', 'eth%d' % (2 * app + 2)))
        config.add_link(
            Link(access1.name, 'eth1', 'agg1-source', 'eth%d' % (2 * app + 2)))
        config.add_link(
            Link(access1.name, 'eth2', 'agg2-sink', 'eth%d' % (2 * app + 2)))
        config.add_link(
            Link(access1.name, 'eth3', 'agg2-source', 'eth%d' % (2 * app + 2)))
        config.add_link(
            Link(access2.name, 'eth0', 'agg1-sink', 'eth%d' % (2 * app + 3)))
        config.add_link(
            Link(access2.name, 'eth1', 'agg1-source', 'eth%d' % (2 * app + 3)))
        config.add_link(
            Link(access2.name, 'eth2', 'agg2-sink', 'eth%d' % (2 * app + 3)))
        config.add_link(
            Link(access2.name, 'eth3', 'agg2-source', 'eth%d' % (2 * app + 3)))

        ## add application hosts and related nodes/links
        for host in range(hosts):
            node = Node('app%d-host%d' % (app, host))
            last = 4 * (host // 2) + 2
            acc_intf_num = host // 2 + 4
            if host % 2 == 0:  # hosts under access1
                access1.add_interface(
                    Interface('eth%d' % acc_intf_num,
                              '10.%d.%d.%d/30' % (second, third, last - 1)))
                node.add_interface(
                    Interface('eth0',
                              '10.%d.%d.%d/30' % (second, third, last)))
                node.add_static_route(
                    Route('0.0.0.0/0',
                          '10.%d.%d.%d' % (second, third, last - 1)))
                config.add_link(
                    Link(node.name, 'eth0', access1.name,
                         'eth%d' % acc_intf_num))
            elif host % 2 == 1:  # hosts under access2
                access2.add_interface(
                    Interface('eth%d' % acc_intf_num,
                              '11.%d.%d.%d/30' % (second, third, last - 1)))
                node.add_interface(
                    Interface('eth0',
                              '11.%d.%d.%d/30' % (second, third, last)))
                node.add_static_route(
                    Route('0.0.0.0/0',
                          '11.%d.%d.%d' % (second, third, last - 1)))
                config.add_link(
                    Link(node.name, 'eth0', access2.name,
                         'eth%d' % acc_intf_num))
            config.add_node(node)

    ## add invariants
    for app in range(apps):
        second = (app // 256) % 256  # second octet
        third = app % 256  # third octet
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
            hosts_other_apps = (('app(%s)-host[0-9]+' %
                                 ('|'.join([str(i) for i in other_apps]))) +
                                '|' + ('access[12]-app(%s)' %
                                       ('|'.join([str(i)
                                                  for i in other_apps]))))
        # In the same application, hosts under access1 can reach hosts under
        # access2
        config.add_invariant(
            Consistency([
                Reachability(target_node=hosts_acc2 + '|access2-app%d' % app,
                             reachable=True,
                             protocol='udp',
                             src_node=hosts_acc1,
                             dst_ip='11.%d.%d.0/24' % (second, third),
                             dst_port=[80],
                             owned_dst_only=True)
            ]))
        # In the same application, hosts under access2 can reach hosts under
        # access1
        config.add_invariant(
            Consistency([
                Reachability(target_node=hosts_acc1 + '|access1-app%d' % app,
                             reachable=True,
                             protocol='udp',
                             src_node=hosts_acc2,
                             dst_ip='10.%d.%d.0/24' % (second, third),
                             dst_port=[80],
                             owned_dst_only=True)
            ]))
        # Hosts of an application cannot reach hosts of other applications
        config.add_invariant(
            Consistency([
                Reachability(target_node=hosts_other_apps,
                             reachable=False,
                             protocol='udp',
                             src_node='app%d-host[0-9]+' % app,
                             dst_ip='10.0.0.0/7',
                             dst_port=[80],
                             owned_dst_only=True)
            ]))
        break

    ## output as TOML
    config.output_toml()


def main():
    parser = argparse.ArgumentParser(description='02-firewall-consistency')
    parser.add_argument('-a',
                        '--apps',
                        type=int,
                        help='Number of applications')
    parser.add_argument('-H',
                        '--hosts',
                        type=int,
                        help='Number of hosts in each application')
    parser.add_argument('-f',
                        '--fault',
                        action='store_true',
                        default=False,
                        help='Use inconsistent rules')
    arg = parser.parse_args()

    if not arg.apps or arg.apps > 65536:
        sys.exit('Invalid number of subnets: ' + str(arg.subnets))
    if not arg.hosts or arg.hosts > 128:
        sys.exit('Invalid number of hosts: ' + str(arg.hosts))

    confgen(arg.apps, arg.hosts, arg.fault)


if __name__ == '__main__':
    main()
