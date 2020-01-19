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
    def __init__(self, name, type='generic'):
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
        data = {'name': self.name, 'type': self.type}
        if self.interfaces:
            data['interfaces'] = [intf.to_dict() for intf in self.interfaces]
        if self.static_routes:
            data['static_routes'] = [route.to_dict()
                                     for route in self.static_routes]
        if self.installed_routes:
            data['installed_routes'] = [route.to_dict()
                                        for route in self.installed_routes]
        return data

class Middlebox(Node):
    def __init__(self, name, env, app):
        Node.__init__(self, name, 'middlebox')
        self.env: str = env
        self.app: str = app
        self.other_configs: Dict = dict()

    def add_config(self, key, value):
        self.other_configs[key] = value

    def to_dict(self):
        data = Node.to_dict(self)
        data.update({'env': self.env, 'app': self.app})
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

class Policy:
    def __init__(self, type, protocol=None, pkt_dst=None, owned_dst_only=None,
                 start_node=None):
        self.type: str            = type
        self.protocol: str        = protocol
        self.pkt_dst: str         = pkt_dst
        self.owned_dst_only: bool = owned_dst_only
        self.start_node: str      = start_node

    def to_dict(self):
        data = self.__dict__
        if self.owned_dst_only == None:
            data.pop('owned_dst_only')
        return data

class ReachabilityPolicy(Policy):
    def __init__(self, protocol, pkt_dst, start_node, final_node, reachable,
                 owned_dst_only=None):
        Policy.__init__(self, 'reachability', protocol, pkt_dst, owned_dst_only,
                        start_node)
        self.final_node: str = final_node
        self.reachable: bool = reachable

class ReplyReachabilityPolicy(Policy):
    def __init__(self, protocol, pkt_dst, start_node, query_node, reachable,
                 owned_dst_only=None):
        Policy.__init__(self, 'reply-reachability', protocol, pkt_dst,
                        owned_dst_only, start_node)
        self.query_node: str = query_node
        self.reachable: bool = reachable

# TODO: add other policy classes

class Policies:
    def __init__(self):
        self.policies: List = list()

    def add_policy(self, policy):
        self.policies.append(policy)

    def to_dict(self):
        if self.policies:
            return {'policies': [policy.to_dict() for policy in self.policies]}
        else:
            return {}
