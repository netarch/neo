#!/usr/bin/python3

from typing import Any, Optional

import toml
import yaml


class Interface:
    def __init__(self, name: str, ipv4: Optional[str] = None):
        self.name: str = name
        self.ipv4: Optional[str] = ipv4

    def to_dict(self):
        return self.__dict__


class Route:
    def __init__(self, network: str, next_hop: str, adm_dist: Optional[int] = None):
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

    def is_bridge(self) -> bool:
        for intf in self.interfaces:
            if intf.ipv4 is not None:
                return False
        return True

    def to_dict(self):
        data = self.__dict__
        if self.interfaces:
            data["interfaces"] = [intf.to_dict() for intf in self.interfaces]
        else:
            data.pop("interfaces")
        if self.static_routes:
            data["static_routes"] = [route.to_dict() for route in self.static_routes]
        else:
            data.pop("static_routes")
        if self.installed_routes:
            data["installed_routes"] = [
                route.to_dict() for route in self.installed_routes
            ]
        else:
            data.pop("installed_routes")
        return data


class Middlebox(Node):
    def __init__(
        self,
        name: str,
        driver: Optional[str] = None,
        start_delay: Optional[int] = None,
        reset_delay: Optional[int] = None,
        replay_delay: Optional[int] = None,
        packets_per_injection: Optional[int] = None,
    ):
        super().__init__(name, "emulation")
        self.driver: Optional[str] = driver
        self.start_delay: Optional[int] = start_delay
        self.reset_delay: Optional[int] = reset_delay
        self.replay_delay: Optional[int] = replay_delay
        self.packets_per_injection: Optional[int] = packets_per_injection


class DockerNode(Middlebox):
    def __init__(
        self,
        name: str,
        image: str,
        working_dir: str,
        start_delay: Optional[int] = None,
        reset_delay: Optional[int] = None,
        replay_delay: Optional[int] = None,
        packets_per_injection: Optional[int] = None,
        dpdk: Optional[bool] = None,
        daemon: Optional[str] = None,
        command: Optional[list[str]] = None,
        args: Optional[list[str]] = None,
        config_files: Optional[list[str]] = None,
    ):
        super().__init__(
            name,
            driver="docker",
            start_delay=start_delay,
            reset_delay=reset_delay,
            replay_delay=replay_delay,
            packets_per_injection=packets_per_injection,
        )

        self.daemon: Optional[str] = daemon
        self.container: dict[str, Any] = dict()
        self.container["image"] = image
        self.container["working_dir"] = working_dir
        self.container["dpdk"] = dpdk
        self.container["command"] = command
        self.container["args"] = args
        self.container["config_files"] = config_files
        self.container["ports"] = list()
        self.container["env"] = list()
        self.container["volume_mounts"] = list()
        self.container["sysctls"] = list()

    def add_port(self, port_no: int, proto: str):
        self.container["ports"].append({"port": port_no, "protocol": proto})

    def add_env_var(self, name: str, value: str):
        self.container["env"].append({"name": name, "value": value})

    def add_volume(
        self,
        mount_path: str,
        host_path: str,
        driver: Optional[str] = None,
        read_only: Optional[bool] = None,
    ):
        self.container["volume_mounts"].append(
            {
                "mount_path": mount_path,
                "host_path": host_path,
                "driver": driver,
                "read_only": read_only,
            }
        )

    def add_sysctl(self, key: str, value: str):
        self.container["sysctls"].append({"key": key, "value": value})

    def to_dict(self):
        data = super().to_dict()
        if not self.container["command"]:
            data["container"].pop("command")
        if not self.container["args"]:
            data["container"].pop("args")
        if not self.container["config_files"]:
            data["container"].pop("config_files")
        if not self.container["ports"]:
            data["container"].pop("ports")
        if not self.container["env"]:
            data["container"].pop("env")
        if not self.container["volume_mounts"]:
            data["container"].pop("volume_mounts")
        if not self.container["sysctls"]:
            data["container"].pop("sysctls")
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
        self.nodes: list[Node | DockerNode] = list()
        self.links: list[Link] = list()

    def is_empty(self):
        return len(self.nodes) + len(self.links) == 0

    def add_node(self, node: Node | DockerNode):
        self.nodes.append(node)

    def add_link(self, link: Link):
        self.links.append(link)

    def to_dict(self):
        data = {}
        if self.nodes:
            data["nodes"] = [node.to_dict() for node in self.nodes]
        if self.links:
            data["links"] = [link.to_dict() for link in self.links]
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
            data["openflow"] = {
                "updates": [update.to_dict() for update in self.updates]
            }
        return data


class Connection:
    def __init__(
        self,
        protocol: str,
        src_node: str,
        dst_ip: str,
        src_port: Optional[int] = None,
        dst_port: Optional[list[int]] = None,
        owned_dst_only: Optional[bool] = None,
    ):
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
            data["connections"] = [conn.to_dict() for conn in self.connections]
        return data


##
## single-connection invariants
##


class Reachability(Invariant):
    def __init__(
        self,
        target_node: str,
        reachable: bool,
        protocol: str,
        src_node: str,
        dst_ip: str,
        src_port: Optional[int] = None,
        dst_port: Optional[list[int]] = None,
        owned_dst_only: Optional[bool] = None,
    ):
        super().__init__("reachability")
        self.target_node: str = target_node
        self.reachable: bool = reachable
        self.add_connection(
            Connection(protocol, src_node, dst_ip, src_port, dst_port, owned_dst_only)
        )


class ReplyReachability(Invariant):
    def __init__(
        self,
        target_node: str,
        reachable: bool,
        protocol: str,
        src_node: str,
        dst_ip: str,
        src_port: Optional[int] = None,
        dst_port: Optional[list[int]] = None,
        owned_dst_only: Optional[bool] = None,
    ):
        super().__init__("reply-reachability")
        self.target_node: str = target_node
        self.reachable: bool = reachable
        self.add_connection(
            Connection(protocol, src_node, dst_ip, src_port, dst_port, owned_dst_only)
        )


class Waypoint(Invariant):
    def __init__(
        self,
        target_node: str,
        pass_through: bool,
        protocol: str,
        src_node: str,
        dst_ip: str,
        src_port: Optional[int] = None,
        dst_port: Optional[list[int]] = None,
        owned_dst_only: Optional[bool] = None,
    ):
        super().__init__("waypoint")
        self.target_node: str = target_node
        self.pass_through: bool = pass_through
        self.add_connection(
            Connection(protocol, src_node, dst_ip, src_port, dst_port, owned_dst_only)
        )


class Loop(Invariant):
    def __init__(
        self,
        protocol: str,
        src_node: str,
        dst_ip: str,
        src_port: Optional[int] = None,
        dst_port: Optional[list[int]] = None,
        owned_dst_only: Optional[bool] = None,
    ):
        super().__init__("loop")
        self.add_connection(
            Connection(protocol, src_node, dst_ip, src_port, dst_port, owned_dst_only)
        )


##
## multi-connection invariants
##


class OneRequest(Invariant):
    def __init__(self, target_node: str):
        super().__init__("one-request")
        self.target_node: str = target_node


class LoadBalance(Invariant):
    def __init__(self, target_node: str, max_dispersion_index: Optional[float] = None):
        super().__init__("loadbalance")
        self.target_node: str = target_node
        self.max_dispersion_index: Optional[float] = max_dispersion_index


##
## invariants with multiple single-connection correlated invariants
##


class Conditional(Invariant):
    def __init__(self, correlated_invs: list[Invariant]):
        super().__init__("conditional")
        self.correlated_invs: list[Invariant] = correlated_invs

    def to_dict(self):
        data = super().to_dict()
        if self.correlated_invs:
            data["correlated_invariants"] = [
                inv.to_dict() for inv in self.correlated_invs
            ]
        data.pop("connections")
        data.pop("correlated_invs")
        return data


class Consistency(Invariant):
    def __init__(self, correlated_invs: list[Invariant]):
        super().__init__("consistency")
        self.correlated_invs: list[Invariant] = correlated_invs

    def to_dict(self):
        data = super().to_dict()
        if self.correlated_invs:
            data["correlated_invariants"] = [
                inv.to_dict() for inv in self.correlated_invs
            ]
        data.pop("connections")
        data.pop("correlated_invs")
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
            return {"invariants": [inv.to_dict() for inv in self.invs]}
        else:
            return {}


class Config:
    def __init__(self):
        self.network = Network()
        self.openflow = Openflow()
        self.invs = Invariants()

    def add_node(self, node: Node | DockerNode):
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

    def deserialize_toml(self, fn: str) -> None:
        self.__init__()
        config = toml.load(fn)
        assert type(config) is dict
        if "nodes" in config:
            for node_cfg in config["nodes"]:
                assert type(node_cfg) is dict
                if "type" in node_cfg and node_cfg["type"] == "emulation":
                    ctnr_cfg = node_cfg["container"]
                    node = DockerNode(
                        name=node_cfg["name"],
                        image=ctnr_cfg["image"],
                        working_dir=ctnr_cfg["working_dir"],
                        start_delay=(
                            node_cfg["start_delay"]
                            if "start_delay" in node_cfg
                            else None
                        ),
                        reset_delay=(
                            node_cfg["reset_delay"]
                            if "reset_delay" in node_cfg
                            else None
                        ),
                        replay_delay=(
                            node_cfg["replay_delay"]
                            if "replay_delay" in node_cfg
                            else None
                        ),
                        packets_per_injection=(
                            node_cfg["packets_per_injection"]
                            if "packets_per_injection" in node_cfg
                            else None
                        ),
                        daemon=(node_cfg["daemon"] if "daemon" in node_cfg else None),
                        # dpdk=(ctnr_cfg["dpdk"] if "dpdk" in ctnr_cfg else None),
                        # command=(
                        #     ctnr_cfg["command"] if "command" in ctnr_cfg else None
                        # ),
                        # args=(ctnr_cfg["args"] if "args" in ctnr_cfg else None),
                        # config_files=(
                        #     ctnr_cfg["config_files"]
                        #     if "config_files" in ctnr_cfg
                        #     else None
                        # ),
                    )
                    # Merge dict ctnr_cfg into node.container
                    node.container = {**node.container, **ctnr_cfg}
                else:
                    node = Node(
                        node_cfg["name"],
                        (node_cfg["type"] if "type" in node_cfg else None),
                    )
                if "interfaces" in node_cfg:
                    for if_cfg in node_cfg["interfaces"]:
                        assert type(if_cfg) is dict
                        node.add_interface(
                            Interface(
                                if_cfg["name"],
                                (if_cfg["ipv4"] if "ipv4" in if_cfg else None),
                            )
                        )
                if "static_routes" in node_cfg:
                    for route_cfg in node_cfg["static_routes"]:
                        assert type(route_cfg) is dict
                        node.add_static_route(
                            Route(
                                route_cfg["network"],
                                route_cfg["next_hop"],
                                (
                                    route_cfg["adm_dist"]
                                    if "adm_dist" in route_cfg
                                    else None
                                ),
                            )
                        )
                if "installed_routes" in node_cfg:
                    for route_cfg in node_cfg["installed_routes"]:
                        assert type(route_cfg) is dict
                        node.add_static_route(
                            Route(
                                route_cfg["network"],
                                route_cfg["next_hop"],
                                (
                                    route_cfg["adm_dist"]
                                    if "adm_dist" in route_cfg
                                    else None
                                ),
                            )
                        )
                self.add_node(node)
        if "links" in config:
            for link_cfg in config["links"]:
                assert type(link_cfg) is dict
                self.add_link(
                    Link(
                        link_cfg["node1"],
                        link_cfg["intf1"],
                        link_cfg["node2"],
                        link_cfg["intf2"],
                    )
                )
        if "invariants" in config:
            for inv_cfg in config["invariants"]:
                assert type(inv_cfg) is dict
                if inv_cfg["type"] == "reachability":
                    assert len(inv_cfg["connections"]) == 1
                    conn_cfg = inv_cfg["connections"][0]
                    assert type(conn_cfg) is dict
                    self.add_invariant(
                        Reachability(
                            target_node=inv_cfg["target_node"],
                            reachable=inv_cfg["reachable"],
                            protocol=conn_cfg["protocol"],
                            src_node=conn_cfg["src_node"],
                            dst_ip=conn_cfg["dst_ip"],
                            src_port=(
                                conn_cfg["src_port"] if "src_port" in conn_cfg else None
                            ),
                            dst_port=(
                                conn_cfg["dst_port"] if "dst_port" in conn_cfg else None
                            ),
                            owned_dst_only=(
                                conn_cfg["owned_dst_only"]
                                if "owned_dst_only" in conn_cfg
                                else None
                            ),
                        )
                    )
                elif inv_cfg["type"] == "reply-reachability":
                    assert len(inv_cfg["connections"]) == 1
                    conn_cfg = inv_cfg["connections"][0]
                    assert type(conn_cfg) is dict
                    self.add_invariant(
                        ReplyReachability(
                            target_node=inv_cfg["target_node"],
                            reachable=inv_cfg["reachable"],
                            protocol=conn_cfg["protocol"],
                            src_node=conn_cfg["src_node"],
                            dst_ip=conn_cfg["dst_ip"],
                            src_port=(
                                conn_cfg["src_port"] if "src_port" in conn_cfg else None
                            ),
                            dst_port=(
                                conn_cfg["dst_port"] if "dst_port" in conn_cfg else None
                            ),
                            owned_dst_only=(
                                conn_cfg["owned_dst_only"]
                                if "owned_dst_only" in conn_cfg
                                else None
                            ),
                        )
                    )
                else:
                    continue  # NOTE: other inv types are not implemented yet.
        # NOTE: skip openflow updates for now.

    # # We may not need to do this with network-mode set to `none`.
    # # https://containerlab.dev/manual/nodes/#network-mode
    # def increment_intf_numbers_by_one_for_clab(self) -> None:
    #     for node in self.network.nodes:
    #         for intf in node.interfaces:
    #             m = re.match(r"eth(\d+)", intf.name)
    #             if m:
    #                 num = int(m.group(1))
    #                 intf.name = "eth{}".format(num + 1)

    def get_bridges(self) -> list[str]:
        bridges = []
        for node in self.network.nodes:
            if node.is_bridge():
                bridges.append(node.name)
        return bridges

    def prefix_all_node_names(self, prefix: str) -> None:
        for node in self.network.nodes:
            node.name = prefix + node.name
        for link in self.network.links:
            link.node1 = prefix + link.node1
            link.node2 = prefix + link.node2
        for inv in self.invs.invs:
            if (
                type(inv) is Reachability
                or type(inv) is ReplyReachability
                or type(inv) is Waypoint
                or type(inv) is OneRequest
                or type(inv) is LoadBalance
            ):
                inv.target_node = prefix + inv.target_node
            for conn in inv.connections:
                conn.src_node = prefix + conn.src_node

    def mangle_l2_switch_intf_names(self) -> None:
        def mangle_intf_name(intf_name: str, node_name: str) -> str:
            if intf_name.startswith("en"):
                intf_name = "en" + node_name.replace("-", "") + intf_name[2:]
            else:
                intf_name = "en" + node_name.replace("-", "") + intf_name
            return intf_name

        for node in self.network.nodes:
            if node.is_bridge():
                for intf in node.interfaces:
                    intf.name = mangle_intf_name(intf.name, node.name)
        br_names = self.get_bridges()
        for link in self.network.links:
            if link.node1 in br_names:
                link.intf1 = mangle_intf_name(link.intf1, link.node1)
            if link.node2 in br_names:
                link.intf2 = mangle_intf_name(link.intf2, link.node2)

    def output_clab_yml(self, name: str, outfn: str | None = None) -> None:
        result = {
            "name": name,
            "prefix": "",
            "topology": {
                "defaults": {"network-mode": "none"},
                "kinds": {
                    "linux": {
                        "sysctls": {
                            "net.ipv4.ip_forward": 1,
                            "net.ipv4.conf.all.forwarding": 1,
                            "net.ipv4.fib_multipath_hash_policy": 1,
                        }
                    }
                },
                "nodes": {},
                "links": [],
            },
        }

        def add_ip_addr(node_cfg: dict, intf: Interface) -> None:
            if "exec" not in node_cfg:
                node_cfg["exec"] = []
            assert intf.ipv4 is not None
            node_cfg["exec"].append(
                "ip addr add {} dev {}".format(intf.ipv4, intf.name)
            )

        for node in self.network.nodes:
            result["topology"]["nodes"][node.name] = dict()
            node_cfg = result["topology"]["nodes"][node.name]
            if node.is_bridge():
                node_cfg["kind"] = "bridge"
                continue
            node_cfg["kind"] = "linux"
            if node.type is None or node.type == "model":
                node_cfg["image"] = "kyechou/linux-router:latest"
            else:
                assert type(node) is DockerNode
                node_cfg["image"] = node.container["image"]
                if "command" in node.container and node.container["command"]:
                    node_cfg["cmd"] = " ".join(node.container["command"])
                if "env" in node.container and node.container["env"]:
                    node_cfg["env"] = dict()
                    for env in node.container["env"]:
                        node_cfg["env"][env["name"]] = (
                            int(env["value"])
                            if env["value"].isnumeric()
                            else env["value"]
                        )
                if "sysctls" in node.container and node.container["sysctls"]:
                    node_cfg["sysctls"] = dict()
                    for var in node.container["sysctls"]:
                        node_cfg["sysctls"][var["key"]] = (
                            int(var["value"])
                            if var["value"].isnumeric()
                            else var["value"]
                        )
            for intf in node.interfaces:
                if intf.ipv4 is not None:
                    add_ip_addr(node_cfg, intf)
            # Aggregate all routes. network prefix -> [next hops]
            all_routes: dict[str, list[str]] = {}
            for route in node.static_routes + node.installed_routes:
                if route.network not in all_routes:
                    all_routes[route.network] = []
                all_routes[route.network].append(route.next_hop)
            for prefix, nhops in all_routes.items():
                cmd = "ip route add {}".format(prefix)
                if len(nhops) == 1:
                    cmd += " via {}".format(nhops[0])
                else:
                    # ECMP
                    assert len(nhops) > 1
                    for nhop in nhops:
                        cmd += " nexthop via {}".format(nhop)
                if "exec" not in node_cfg:
                    node_cfg["exec"] = []
                node_cfg["exec"].append(cmd)

        for link in self.network.links:
            result["topology"]["links"].append(
                {
                    "endpoints": [
                        "{}:{}".format(link.node1, link.intf1),
                        "{}:{}".format(link.node2, link.intf2),
                    ]
                }
            )

        if outfn:
            with open(outfn, "w") as fout:
                yaml.dump(result, fout)
        else:
            yml_str = yaml.dump(result)
            print(yml_str)

    def output_invariants(self) -> None:
        for inv in self.invs.invs:
            if type(inv) is not Reachability and type(inv) is not ReplyReachability:
                continue
            assert len(inv.connections) == 1
            conn: Connection = inv.connections[0]
            if conn.owned_dst_only:
                continue
            # target_node, reachable, protocol, src_node, dst_ip
            print(
                "{},{},{},{},{}".format(
                    inv.target_node,
                    inv.reachable,
                    conn.protocol,
                    conn.src_node,
                    conn.dst_ip,
                )
            )
