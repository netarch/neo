#!/usr/bin/python3

import argparse
import os
import random
import sys
from collections import deque

sys.path.append(os.path.join(os.path.dirname(__file__), "../.."))
from src.config import (
    Config,
    DockerNode,
    Interface,
    Link,
    Loop,
    Node,
    Reachability,
    Route,
)

sys.path.append(os.path.join(os.path.dirname(__file__), "data"))
from parser.device import Device as DDevice
from parser.interface import Interface as DInterface
from parser.network import Network as DNetwork
from parser.parser import Parser as DParser

# from .data.parser.device import Device as DDevice
# from .data.parser.interface import Interface as DInterface
# from .data.parser.network import Network as DNetwork
# from .data.parser.parser import Parser as DParser


def device_use_acls(device: DDevice) -> bool:
    for intf in device.interfaces.values():
        for acl_id in intf.acls.keys():
            if acl_id in device.access_lists:
                return True
    return False


def get_device_names_with_acls(network: DNetwork) -> list[str]:
    results: list[str] = []
    for device_name, device in network.nodes.items():
        if device_use_acls(device):
            results.append(device_name)
    return results


def get_iptables_acl_rules(device: DDevice) -> str:
    rules = (
        "*filter\n"
        ":INPUT ACCEPT [0:0]\n"
        ":FORWARD DROP [0:0]\n"
        ":OUTPUT ACCEPT [0:0]\n"
    )
    for intf in device.interfaces.values():
        for acl_id, direction in intf.acls.items():
            if acl_id not in device.access_lists:
                continue
            acl = device.access_lists[acl_id]
            for acl_rule in acl.list:
                new_rule = "-A FORWARD"
                if direction == "in":
                    new_rule += " -i {}".format(intf.get_name())
                elif direction == "out":
                    new_rule += " -o {}".format(intf.get_name())
                new_rule += " -s {}".format(str(acl_rule.source))
                if acl_rule.destination is not None:
                    new_rule += " -d {}".format(str(acl_rule.destination))
                if acl_rule.action == "permit":
                    new_rule += " -j ACCEPT\n"
                else:
                    new_rule += " -j DROP\n"
                rules += new_rule
    return rules


def add_l2_switch(config: Config, switch_name: str, num_intfs: int) -> None:
    node = Node(name=switch_name, type="model")
    for intf_id in range(num_intfs):
        node.add_interface(Interface(name="en{}".format(intf_id)))
    config.add_node(node)


def is_connected(network: DNetwork, src_device_name: str, dst_device_name: str) -> bool:
    q: deque[str] = deque()
    q.append(src_device_name)
    visited: set[str] = set()
    while len(q) > 0:
        device_name = q.popleft()
        if device_name == dst_device_name:
            return True
        visited.add(device_name)
        # Add unvisited neighbors to the queue
        device = network.nodes[device_name]
        for intf in device.interfaces.values():
            if (device, intf) not in network.links:
                continue
            for neighbor_ep in network.links[(device, intf)]:
                neighbor = neighbor_ep[0].name
                if neighbor not in visited:
                    q.append(neighbor)
    return False


def confgen(network_name: str, max_invs: int, loop: bool) -> None:
    config = Config()
    random.seed(42)  # Fixed seed for reproducibility

    network = DParser.parse(
        os.path.join(os.path.dirname(__file__), "data"), network_name
    )
    emulated_device_names = get_device_names_with_acls(network)
    # emulated_device_names = random.sample(
    #     list(network.nodes.keys()), k=int(len(network.nodes) * emu_pct / 100)
    # )

    # Add nodes
    for device_name, device in network.nodes.items():
        if device_name in emulated_device_names:
            # Emulated node, but not a firewall
            has_acl = device_use_acls(device)
            newnode = DockerNode(
                name=device_name,
                image=(
                    "kyechou/iptables:latest"
                    if has_acl
                    else "kyechou/linux-router:latest"
                ),
                working_dir="/",
                command=["/start.sh"],
            )
            if has_acl:
                rules = get_iptables_acl_rules(device)
                newnode.add_env_var("RULES", rules)
            newnode.add_sysctl("net.ipv4.conf.all.forwarding", "1")
            newnode.add_sysctl("net.ipv4.conf.all.rp_filter", "0")
            newnode.add_sysctl("net.ipv4.conf.default.rp_filter", "0")
        else:
            # Not an emulated node
            newnode = Node(name=device_name, type="model")

        for if_name, intf in device.interfaces.items():
            ip_str = str(intf.ip_addr) if intf.ip_addr is not None else None
            newnode.add_interface(Interface(if_name, ip_str))
        for prefix, route in device.routes.items():
            if route.next_hop_ip is None:
                continue
            newnode.add_static_route(
                Route(str(prefix), str(route.next_hop_ip), route.adm_dist)
            )
        config.add_node(newnode)

    # Add links
    switch_id: int = 0
    visited_endpoints: set[tuple[DDevice, DInterface]] = set()
    for from_ep, to_ep_set in network.links.items():
        if from_ep in visited_endpoints:
            continue
        if len(to_ep_set) == 0:
            continue
        elif len(to_ep_set) == 1:
            to_ep = next(iter(to_ep_set))
            if to_ep in visited_endpoints:
                continue
            config.add_link(
                Link(
                    from_ep[0].name,
                    from_ep[1].get_name(),
                    to_ep[0].name,
                    to_ep[1].get_name(),
                )
            )
        else:
            # Multiple (more than 2) L3 endpoints connected with a L2 switch.
            switch_name = "sw-{}".format(switch_id)
            switch_id += 1
            endpoints = [from_ep] + list(to_ep_set)
            add_l2_switch(config, switch_name, len(endpoints))
            for id, ep in enumerate(endpoints):
                if ep in visited_endpoints:
                    continue
                config.add_link(
                    Link(
                        switch_name,
                        "en{}".format(id),
                        ep[0].name,
                        ep[1].get_name(),
                    )
                )
        visited_endpoints.add(from_ep)
        visited_endpoints = visited_endpoints.union(to_ep_set)

    # Add invariants
    num_invs = 0
    if loop and max_invs >= 1:
        config.add_invariant(
            Loop(
                protocol="icmp-echo",
                src_node=".*",
                dst_ip="0.0.0.0/0",
            )
        )
        num_invs += 1
    # all_device_names = list(network.nodes.keys())
    # random.shuffle(all_device_names)
    bd_device_names: list[str] = [
        device_name
        for device_name in network.nodes.keys()
        if device_name.startswith("bd-")
    ]
    random.shuffle(bd_device_names)
    for u in bd_device_names:
        if num_invs >= max_invs:
            break
        for v in bd_device_names:
            if num_invs >= max_invs:
                break
            if u == v:
                continue
            if u in emulated_device_names or v in emulated_device_names:
                continue
            config.add_invariant(
                Reachability(
                    target_node=v,
                    reachable=is_connected(network, u, v),
                    protocol="icmp-echo",
                    src_node=u,
                    dst_ip="0.0.0.0/0",
                    owned_dst_only=True,
                )
            )
            num_invs += 1

    config.output_toml()


def main():
    parser = argparse.ArgumentParser(description="17-campus-network")
    parser.add_argument(
        "-n",
        "--network",
        type=str,
        help="Network name",
        required=True,
        choices=[
            "core1",
            "core2",
            "core4",
            "core5",
            "core6",
            "core8",
            "core9",
            "core10",
            "all",
        ],
    )
    # parser.add_argument(
    #     "-e", "--emulated", type=int, help="Percentage of emulated nodes", required=True
    # )
    # parser.add_argument(
    #     "-i", "--invs", type=int, help="Maximum number of invariants", required=True
    # )
    parser.add_argument(
        "-l",
        "--loop",
        action="store_true",
        help="Whether to include a loop invariant",
        default=False,
    )
    arg = parser.parse_args()

    confgen(arg.network, max_invs=sys.maxsize, loop=arg.loop)


if __name__ == "__main__":
    main()
