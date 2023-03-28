#!/usr/bin/python3

from typing import List
from typing import Dict
import toml


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
        self.network: str = network
        self.next_hop: str = next_hop
        self.adm_dist: int = adm_dist

    def to_dict(self):
        data = {'network': self.network, 'next_hop': self.next_hop}
        if self.adm_dist != None:
            data['adm_dist'] = self.adm_dist
        return data


class Node:

    def __init__(self, name, type=None):
        self.name: str = name
        self.type: str = type
        self.interfaces: List[Interface] = list()
        self.static_routes: List[Route] = list()
        self.installed_routes: List[Route] = list()

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
            data['static_routes'] = [
                route.to_dict() for route in self.static_routes
            ]
        if self.installed_routes:
            data['installed_routes'] = [
                route.to_dict() for route in self.installed_routes
            ]
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

    def is_empty(self):
        return len(self.nodes) + len(self.links) == 0

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

    def is_empty(self):
        return len(self.updates) == 0

    def add_update(self, update):
        self.updates.append(update)

    def to_dict(self):
        data = {}
        if self.updates:
            data['openflow'] = {
                'updates': [update.to_dict() for update in self.updates]
            }
        return data


class Connection:

    def __init__(self,
                 protocol,
                 src_node,
                 dst_ip,
                 src_port=None,
                 dst_port=None,
                 owned_dst_only=None):
        self.protocol: str = protocol
        self.src_node: str = src_node
        self.dst_ip: str = dst_ip
        self.src_port: int = src_port
        self.dst_port: List[int] = dst_port
        self.owned_dst_only: bool = owned_dst_only

    def to_dict(self):
        data = self.__dict__
        if self.src_port is None:
            data.pop('src_port')
        if self.dst_port is None:
            data.pop('dst_port')
        if self.owned_dst_only is None:
            data.pop('owned_dst_only')
        return data


class Invariant:

    def __init__(self, type):
        self.type: str = type
        self.connections: List[Connection] = list()

    def add_connection(self, conn):
        self.connections.append(conn)

    def to_dict(self):
        data = self.__dict__
        if self.connections:
            data['connections'] = [conn.to_dict() for conn in self.connections]
        return data


##
## single-connection invariants
##


class Reachability(Invariant):

    def __init__(self,
                 target_node,
                 reachable,
                 protocol,
                 src_node,
                 dst_ip,
                 src_port=None,
                 dst_port=None,
                 owned_dst_only=None):
        Invariant.__init__(self, 'reachability')
        self.target_node: str = target_node
        self.reachable: bool = reachable
        self.add_connection(
            Connection(protocol, src_node, dst_ip, src_port, dst_port,
                       owned_dst_only))


class ReplyReachability(Invariant):

    def __init__(self,
                 target_node,
                 reachable,
                 protocol,
                 src_node,
                 dst_ip,
                 src_port=None,
                 dst_port=None,
                 owned_dst_only=None):
        Invariant.__init__(self, 'reply-reachability')
        self.target_node: str = target_node
        self.reachable: bool = reachable
        self.add_connection(
            Connection(protocol, src_node, dst_ip, src_port, dst_port,
                       owned_dst_only))


class Waypoint(Invariant):

    def __init__(self,
                 target_node,
                 pass_through,
                 protocol,
                 src_node,
                 dst_ip,
                 src_port=None,
                 dst_port=None,
                 owned_dst_only=None):
        Invariant.__init__(self, 'waypoint')
        self.target_node: str = target_node
        self.pass_through: bool = pass_through
        self.add_connection(
            Connection(protocol, src_node, dst_ip, src_port, dst_port,
                       owned_dst_only))


##
## multi-connection invariants
##


class OneRequest(Invariant):

    def __init__(self, target_node):
        Invariant.__init__(self, 'one-request')
        self.target_node: str = target_node


class LoadBalance(Invariant):

    def __init__(self, target_node, max_dispersion_index=None):
        Invariant.__init__(self, 'loadbalance')
        self.target_node: str = target_node
        self.max_dispersion_index: int = max_dispersion_index

    def to_dict(self):
        data = Invariant.to_dict(self)
        if self.max_dispersion_index is None:
            data.pop('max_dispersion_index')
        return data


##
## invariants with multiple single-connection correlated invariants
##


class Conditional(Invariant):

    def __init__(self, correlated_invs):
        Invariant.__init__(self, 'conditional')
        self.correlated_invs: List[Invariant] = correlated_invs

    def to_dict(self):
        data = Invariant.to_dict(self)
        if self.correlated_invs:
            data['correlated_invariants'] = [
                inv.to_dict() for inv in self.correlated_invs
            ]
        return data


class Consistency(Invariant):

    def __init__(self, correlated_invs):
        Invariant.__init__(self, 'consistency')
        self.correlated_invs: List[Invariant] = correlated_invs

    def to_dict(self):
        data = Invariant.to_dict(self)
        if self.correlated_invs:
            data['correlated_invariants'] = [
                inv.to_dict() for inv in self.correlated_invs
            ]
        return data


class Invariants:

    def __init__(self):
        self.invs: List[Invariant] = list()

    def is_empty(self):
        return len(self.invs) == 0

    def add_invariant(self, inv):
        self.invs.append(inv)

    def to_dict(self):
        if self.invs:
            return {'invariants': [inv.to_dict() for inv in self.invs]}
        else:
            return {}


class Config:

    def __init__(self):
        self.network = Network()
        self.openflow = Openflow()
        self.invs = Invariants()

    def add_node(self, node):
        self.network.add_node(node)

    def add_link(self, link):
        self.network.add_link(link)

    def add_update(self, update):
        self.openflow.add_update(update)

    def add_invariant(self, inv):
        self.invs.add_invariant(inv)

    def output_toml(self):
        config = {}
        if not self.network.is_empty():
            config.update(self.network.to_dict())
        if not self.openflow.is_empty():
            config.update(self.openflow.to_dict())
        if not self.invs.is_empty():
            config.update(self.invs.to_dict())
        print(toml.dumps(config))
