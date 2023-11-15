#!/usr/bin/python3

from typing import Any
from typing import Optional
import toml


class Interface:

    def __init__(self, name: str, ipv4: Optional[str] = None):
        self.name: str = name
        self.ipv4: Optional[str] = ipv4

    def to_dict(self):
        return self.__dict__


class Route:

    def __init__(self,
                 network: str,
                 next_hop: str,
                 adm_dist: Optional[int] = None):
        self.network: str = network
        self.next_hop: str = next_hop
        self.adm_dist: Optional[int] = adm_dist

    def to_dict(self):
        return self.__dict__


class Node:

    def __init__(self, name: str, type: Optional[str] = None):
        self.name: str = name
        self.type: Optional[str] = type
        self.interfaces: list[Interface] = list()
        self.static_routes: list[Route] = list()
        self.installed_routes: list[Route] = list()

    def add_interface(self, intf: Interface):
        self.interfaces.append(intf)

    def add_static_route(self, route: Route):
        self.static_routes.append(route)

    def add_installed_route(self, route: Route):
        self.installed_routes.append(route)

    def to_dict(self):
        data = self.__dict__
        if self.interfaces:
            data['interfaces'] = [intf.to_dict() for intf in self.interfaces]
        else:
            data.pop('interfaces')
        if self.static_routes:
            data['static_routes'] = [
                route.to_dict() for route in self.static_routes
            ]
        else:
            data.pop('static_routes')
        if self.installed_routes:
            data['installed_routes'] = [
                route.to_dict() for route in self.installed_routes
            ]
        else:
            data.pop('installed_routes')
        return data


class Middlebox(Node):

    def __init__(self, name: str, driver: str):
        super().__init__(name, 'emulation')
        self.driver: str = driver


class DockerNode(Middlebox):

    def __init__(self,
                 name: str,
                 image: str,
                 working_dir: str,
                 dpdk: Optional[bool] = None,
                 start_wait_time: Optional[int] = None,
                 reset_wait_time: Optional[int] = None,
                 daemon: Optional[str] = None,
                 command: Optional[list[str]] = None,
                 args: Optional[list[str]] = None,
                 config_files: Optional[list[str]] = None):
        super().__init__(name, 'docker')

        self.daemon: Optional[str] = daemon
        self.container: dict[str, Any] = dict()
        self.container['image']: str = image
        self.container['working_dir']: str = working_dir
        self.container['dpdk']: Optional[bool] = dpdk
        self.container['start_wait_time']: Optional[int] = start_wait_time
        self.container['reset_wait_time']: Optional[int] = reset_wait_time
        self.container['command']: Optional[list[str]] = command
        self.container['args']: Optional[list[str]] = args
        self.container['config_files']: Optional[list[str]] = config_files
        self.container['ports']: list[dict[str, str | int]] = list()
        self.container['env']: list[dict[str, str]] = list()
        self.container['volume_mounts']: list[dict[str,
                                                   Optional[str
                                                            | bool]]] = list()
        self.container['sysctls']: list[dict[str, str]] = list()

    def add_port(self, port_no: int, proto: str):
        self.container['ports'].append({'port': port_no, 'protocol': proto})

    def add_env_var(self, name: str, value: str):
        self.container['env'].append({'name': name, 'value': value})

    def add_volume(self,
                   mount_path: str,
                   host_path: str,
                   driver: Optional[str] = None,
                   read_only: Optional[bool] = None):
        self.container['volume_mounts'].append({
            'mount_path': mount_path,
            'host_path': host_path,
            'driver': driver,
            'read_only': read_only
        })

    def add_sysctl(self, key: str, value: str):
        self.container['sysctls'].append({'key': key, 'value': value})

    def to_dict(self):
        data = super().to_dict()
        if not self.container['command']:
            data['container'].pop('command')
        if not self.container['args']:
            data['container'].pop('args')
        if not self.container['config_files']:
            data['container'].pop('config_files')
        if not self.container['ports']:
            data['container'].pop('ports')
        if not self.container['env']:
            data['container'].pop('env')
        if not self.container['volume_mounts']:
            data['container'].pop('volume_mounts')
        if not self.container['sysctls']:
            data['container'].pop('sysctls')
        return data


class Link:

    def __init__(self, node1: str, intf1: str, node2: str, intf2: str):
        self.node1: str = node1
        self.intf1: str = intf1
        self.node2: str = node2
        self.intf2: str = intf2

    def to_dict(self):
        return self.__dict__


class Network:

    def __init__(self):
        self.nodes: list[Node] = list()
        self.links: list[Link] = list()

    def is_empty(self):
        return len(self.nodes) + len(self.links) == 0

    def add_node(self, node: Node):
        self.nodes.append(node)

    def add_link(self, link: Link):
        self.links.append(link)

    def to_dict(self):
        data = {}
        if self.nodes:
            data['nodes'] = [node.to_dict() for node in self.nodes]
        if self.links:
            data['links'] = [link.to_dict() for link in self.links]
        return data


class OpenflowUpdate:

    def __init__(self, node_name: str, network: str, outport: str):
        self.node: str = node_name
        self.network: str = network
        self.outport: str = outport

    def to_dict(self):
        return self.__dict__


class Openflow:

    def __init__(self):
        self.updates: list[OpenflowUpdate] = list()

    def is_empty(self):
        return len(self.updates) == 0

    def add_update(self, update: OpenflowUpdate):
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
                 protocol: str,
                 src_node: str,
                 dst_ip: str,
                 src_port: Optional[int] = None,
                 dst_port: Optional[list[int]] = None,
                 owned_dst_only: Optional[bool] = None):
        self.protocol: str = protocol
        self.src_node: str = src_node
        self.dst_ip: str = dst_ip
        self.src_port: Optional[int] = src_port
        self.dst_port: Optional[list[int]] = dst_port
        self.owned_dst_only: Optional[bool] = owned_dst_only

    def to_dict(self):
        return self.__dict__


class Invariant:

    def __init__(self, type: str):
        self.type: str = type
        self.connections: list[Connection] = list()

    def add_connection(self, conn: Connection):
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
                 target_node: str,
                 reachable: bool,
                 protocol: str,
                 src_node: str,
                 dst_ip: str,
                 src_port: Optional[int] = None,
                 dst_port: Optional[list[int]] = None,
                 owned_dst_only: Optional[bool] = None):
        super().__init__('reachability')
        self.target_node: str = target_node
        self.reachable: bool = reachable
        self.add_connection(
            Connection(protocol, src_node, dst_ip, src_port, dst_port,
                       owned_dst_only))


class ReplyReachability(Invariant):

    def __init__(self,
                 target_node: str,
                 reachable: bool,
                 protocol: str,
                 src_node: str,
                 dst_ip: str,
                 src_port: Optional[int] = None,
                 dst_port: Optional[list[int]] = None,
                 owned_dst_only: Optional[bool] = None):
        super().__init__('reply-reachability')
        self.target_node: str = target_node
        self.reachable: bool = reachable
        self.add_connection(
            Connection(protocol, src_node, dst_ip, src_port, dst_port,
                       owned_dst_only))


class Waypoint(Invariant):

    def __init__(self,
                 target_node: str,
                 pass_through: bool,
                 protocol: str,
                 src_node: str,
                 dst_ip: str,
                 src_port: Optional[int] = None,
                 dst_port: Optional[list[int]] = None,
                 owned_dst_only: Optional[bool] = None):
        super().__init__('waypoint')
        self.target_node: str = target_node
        self.pass_through: bool = pass_through
        self.add_connection(
            Connection(protocol, src_node, dst_ip, src_port, dst_port,
                       owned_dst_only))


##
## multi-connection invariants
##


class OneRequest(Invariant):

    def __init__(self, target_node: str):
        super().__init__('one-request')
        self.target_node: str = target_node


class LoadBalance(Invariant):

    def __init__(self,
                 target_node: str,
                 max_dispersion_index: Optional[float] = None):
        super().__init__('loadbalance')
        self.target_node: str = target_node
        self.max_dispersion_index: Optional[float] = max_dispersion_index


##
## invariants with multiple single-connection correlated invariants
##


class Conditional(Invariant):

    def __init__(self, correlated_invs: list[Invariant]):
        super().__init__('conditional')
        self.correlated_invs: list[Invariant] = correlated_invs

    def to_dict(self):
        data = super().to_dict()
        if self.correlated_invs:
            data['correlated_invariants'] = [
                inv.to_dict() for inv in self.correlated_invs
            ]
        return data


class Consistency(Invariant):

    def __init__(self, correlated_invs: list[Invariant]):
        super().__init__('consistency')
        self.correlated_invs: list[Invariant] = correlated_invs

    def to_dict(self):
        data = super().to_dict()
        if self.correlated_invs:
            data['correlated_invariants'] = [
                inv.to_dict() for inv in self.correlated_invs
            ]
        return data


class Invariants:

    def __init__(self):
        self.invs: list[Invariant] = list()

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

    def add_node(self, node: Node):
        self.network.add_node(node)

    def add_link(self, link: Link):
        self.network.add_link(link)

    def add_update(self, update: OpenflowUpdate):
        self.openflow.add_update(update)

    def add_invariant(self, inv: Invariant):
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
