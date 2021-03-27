#!/usr/bin/python3

from typing import List
from typing import Dict

class Interface:
    def __init__(self, name, ipv4=None):
        self.name: str = name
        self.ipv4: str = ipv4

    def to_dict(self):
        data = {'name': self.name}
        if self.ipv4 != None:
            data['ipv4'] = self.ipv4
        return data

class Route:
    def __init__(self, network, next_hop, adm_dist=None):
        self.network: str  = network
        self.next_hop: str = next_hop
        self.adm_dist: int = adm_dist

    def to_dict(self):
        data = {'network': self.network, 'next_hop': self.next_hop}
        if self.adm_dist != None:
            data['adm_dist'] = self.adm_dist
        return data

class Node:
    def __init__(self, name, type=None):
        self.name: str                      = name
        self.type: str                      = type
        self.interfaces: List[Interface]    = list()
        self.static_routes: List[Route]     = list()
        self.installed_routes: List[Route]  = list()

    def add_interface(self, intf):
        self.interfaces.append(intf)

    def add_static_route(self, route):
        self.static_routes.append(route)

    def add_installed_route(self, route):
        self.installed_routes.append(route)

    def to_dict(self):
        data = {'name': self.name}
        if self.type != None:
            data['type'] = self.type
        if self.interfaces:
            data['interfaces'] = [intf.to_dict() for intf in self.interfaces]
        if self.static_routes:
            data['static_routes'] = [route.to_dict() for route in self.static_routes]
        if self.installed_routes:
            data['installed_routes'] = [route.to_dict() for route in self.installed_routes]
        return data

class Middlebox(Node):
    def __init__(self, name, env, app):
        Node.__init__(self, name, 'middlebox')
        self.env: str = env
        self.app: str = app
        self.timeout: int = None
        self.other_configs: Dict = dict()

    def set_timeout(self, timeout):
        self.timeout = timeout

    def add_config(self, key, value):
        self.other_configs[key] = value

    def to_dict(self):
        data = Node.to_dict(self)
        data.update({'env': self.env, 'app': self.app})
        if self.timeout != None:
            data.update({'timeout': self.timeout})
        data.update(self.other_configs)
        return data

class Link:
    def __init__(self, node1, intf1, node2, intf2):
        self.node1: str = node1
        self.intf1: str = intf1
        self.node2: str = node2
        self.intf2: str = intf2

    def to_dict(self):
        return self.__dict__

class Network:
    def __init__(self):
        self.nodes: List[Node] = list()
        self.links: List[Link] = list()

    def add_node(self, node):
        self.nodes.append(node)

    def add_link(self, link):
        self.links.append(link)

    def to_dict(self):
        data = {}
        if self.nodes:
            data['nodes'] = [node.to_dict() for node in self.nodes]
        if self.links:
            data['links'] = [link.to_dict() for link in self.links]
        return data

class OpenflowUpdate:
    def __init__(self, node_name, network, outport):
        self.node: str = node_name
        self.network: str = network
        self.outport: str = outport

    def to_dict(self):
        return self.__dict__

class Openflow:
    def __init__(self):
        self.updates: List[OpenflowUpdate] = list()

    def add_update(self, update):
        self.updates.append(update);

    def to_dict(self):
        data = {}
        if self.updates:
            data['openflow'] = {'updates': [update.to_dict() for update in self.updates]}
        return data

class Communication:
    def __init__(self, protocol=None, pkt_dst=None, dst_port=None,
                 owned_dst_only=None, start_node=None):
        self.protocol: str        = protocol
        self.pkt_dst: str         = pkt_dst
        self.dst_port: int        = dst_port
        self.owned_dst_only: bool = owned_dst_only
        self.start_node: str      = start_node

    def to_dict(self):
        data = self.__dict__
        if self.protocol is None:
            data.pop('protocol')
        if self.pkt_dst is None:
            data.pop('pkt_dst')
        if self.dst_port is None:
            data.pop('dst_port')
        if self.owned_dst_only is None:
            data.pop('owned_dst_only')
        if self.start_node is None:
            data.pop('start_node')
        return data

class Policy:
    def __init__(self, type):
        self.type: str = type

##
## single-communication policies
##

class LoadBalancePolicy(Policy):
    def __init__(self, protocol, pkt_dst, start_node, final_node, repeat,
                 dst_port=None, owned_dst_only=None):
        Policy.__init__(self, 'loadbalance')
        self.final_node: str = final_node
        self.repeat: int = repeat
        self.communication = Communication(protocol, pkt_dst, dst_port,
                                           owned_dst_only, start_node)

    def to_dict(self):
        data = self.__dict__
        if self.repeat is None:
            data.pop('repeat')
        data['communication'] = self.communication.to_dict()
        return data

class ReachabilityPolicy(Policy):
    def __init__(self, protocol, pkt_dst, start_node, final_node, reachable,
                 dst_port=None, owned_dst_only=None):
        Policy.__init__(self, 'reachability')
        self.final_node: str = final_node
        self.reachable: bool = reachable
        self.communication = Communication(protocol, pkt_dst, dst_port,
                                           owned_dst_only, start_node)

    def to_dict(self):
        data = self.__dict__
        data['communication'] = self.communication.to_dict()
        return data

class ReplyReachabilityPolicy(Policy):
    def __init__(self, protocol, pkt_dst, start_node, query_node, reachable,
                 dst_port=None, owned_dst_only=None):
        Policy.__init__(self, 'reply-reachability')
        self.query_node: str = query_node
        self.reachable: bool = reachable
        self.communication = Communication(protocol, pkt_dst, dst_port,
                                           owned_dst_only, start_node)

    def to_dict(self):
        data = self.__dict__
        data['communication'] = self.communication.to_dict()
        return data

class WaypointPolicy(Policy):
    def __init__(self, protocol, pkt_dst, start_node, waypoint, pass_through,
                 dst_port=None, owned_dst_only=None):
        Policy.__init__(self, 'waypoint')
        self.waypoint: str = waypoint
        self.pass_through: bool = pass_through
        self.communication = Communication(protocol, pkt_dst, dst_port,
                                           owned_dst_only, start_node)

    def to_dict(self):
        data = self.__dict__
        data['communication'] = self.communication.to_dict()
        return data

##
## multi-communication policies
##

# TODO: OneRequestPolicy (one-request)

class MulticommLBPolicy(Policy):
    def __init__(self, final_node, max_dispersion_index=None):
        Policy.__init__(self, 'multicomm-lb')
        self.final_node: str = final_node
        self.max_dispersion_index: float = max_dispersion_index
        self.communications: List[Communication] = list()

    def add_communication(self, comm):
        self.communications.append(comm)

    def to_dict(self):
        data = self.__dict__
        if self.max_dispersion_index is None:
            data.pop('max_dispersion_index')
        if self.communications:
            data['communications'] = [comm.to_dict() for comm in self.communications]
        return data

##
## policies with multiple single-communication correlated policies
##

class ConditionalPolicy(Policy):
    def __init__(self, correlated_policies):
        Policy.__init__(self, 'conditional')
        self.correlated_policies: List[Policy] = correlated_policies

    def to_dict(self):
        data = self.__dict__
        if self.correlated_policies:
            data['correlated_policies'] = [policy.to_dict()
                    for policy in self.correlated_policies]
        return data

class ConsistencyPolicy(Policy):
    def __init__(self, correlated_policies):
        Policy.__init__(self, 'consistency')
        self.correlated_policies: List[Policy] = correlated_policies

    def to_dict(self):
        data = self.__dict__
        if self.correlated_policies:
            data['correlated_policies'] = [policy.to_dict()
                    for policy in self.correlated_policies]
        return data

class Policies:
    def __init__(self):
        self.policies: List[Policy] = list()

    def add_policy(self, policy):
        self.policies.append(policy)

    def to_dict(self):
        if self.policies:
            return {'policies': [policy.to_dict() for policy in self.policies]}
        else:
            return {}

import toml

def output_toml(network, openflow, policies):
    config = {}
    if network != None:
        config.update(network.to_dict())
    if openflow != None:
        config.update(openflow.to_dict())
    if policies != None:
        config.update(policies.to_dict())
    print(toml.dumps(config))
