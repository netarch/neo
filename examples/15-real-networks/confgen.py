import argparse
import os
import random
import sys

import pandas as pd

sys.path.append(os.path.join(os.path.dirname(__file__), "../.."))
from src.config import (
    Config,
    DockerNode,
    Interface,
    Link,
    Node,
    Reachability,
    Route,
)
from src.dataparse import NetSynthesizer, bfs_is_connected


class FirewallData:
    def __init__(self) -> None:
        base_dir = os.path.abspath(os.path.dirname(__file__))
        data_dir = os.path.join(base_dir, "data")
        self.df = pd.read_csv(os.path.join(data_dir, "firewall-data-aggregated.csv"))

    def get_iptables_rules(self, intf: str) -> str:
        rules = (
            "*filter\n"
            ":INPUT ACCEPT [0:0]\n"
            ":FORWARD DROP [0:0]\n"
            ":OUTPUT ACCEPT [0:0]\n"
        )
        for _, row in self.df.iterrows():
            new_rule = "-A FORWARD -p udp -o {}".format(intf)
            if row["sport"] != -1:
                new_rule += " --sport {}".format(row["sport"])
            if row["dport"] != -1:
                new_rule += " --dport {}".format(row["dport"])
            new_rule += " -j {}\n".format(
                "ACCEPT" if row["action"] == "allow" else "DROP"
            )
            rules += new_rule
        # print(rules, file=sys.stderr)
        return rules

    # Returns: (sport, dport, action)
    def get_random_rule(self) -> tuple[int | None, int, str]:
        row = self.df[self.df.dport != -1].sample().iloc[0]
        return (
            (int(row.sport) if row.sport != -1 else None),
            int(row.dport),
            row.action,
        )


def confgen(
    topo_file_name: str, emu_percentage: int, fw_percentage: int, max_invs: int
):
    random.seed(42)  # Fixed seed for reproducibility
    config = Config()
    ns = NetSynthesizer(topo_file_name)
    fwdata = FirewallData()

    leaves = ns.leaves()
    leaves_behind_firewalls = random.sample(
        leaves, k=int(len(leaves) * fw_percentage / 100)
    )
    leaves_wo_firewalls = list(set(leaves) - set(leaves_behind_firewalls))
    firewalls = []
    for leaf in leaves_behind_firewalls:
        firewalls.append(ns.get_firewall(leaf))
    non_firewalls = list(set(ns.node_to_interfaces.keys()) - set(firewalls))
    max_num_emu_nodes = int(len(ns.node_to_interfaces) * emu_percentage / 100)
    emulated = random.sample(
        non_firewalls,
        k=(
            (max_num_emu_nodes - len(firewalls))
            if (max_num_emu_nodes - len(firewalls)) > 0
            else 0
        ),
    )
    emulated_node_names = set()

    for i, n in enumerate(ns.node_to_interfaces):
        if n in firewalls:
            # Firewall (emulated) node
            emulated_node_names.add(n)
            newnode = DockerNode(
                n,
                image="kyechou/iptables:latest",
                working_dir="/",
                command=["/start.sh"],
            )
            intf = ns.get_itf_to_leaf(n)
            assert intf is not None
            rules = fwdata.get_iptables_rules(intf)
            newnode.add_env_var("RULES", rules)
            newnode.add_sysctl("net.ipv4.conf.all.forwarding", "1")
            newnode.add_sysctl("net.ipv4.conf.all.rp_filter", "0")
            newnode.add_sysctl("net.ipv4.conf.default.rp_filter", "0")
        elif i in emulated:
            # Emulated node, but not a firewall
            emulated_node_names.add(n)
            newnode = DockerNode(
                n,
                image="kyechou/linux-router:latest",
                working_dir="/",
                command=["/start.sh"],
            )
            newnode.add_sysctl("net.ipv4.conf.all.forwarding", "1")
            newnode.add_sysctl("net.ipv4.conf.all.rp_filter", "0")
            newnode.add_sysctl("net.ipv4.conf.default.rp_filter", "0")
        else:
            # Not an emulated node
            newnode = Node(n, type="model")

        for j in ns.node_to_interfaces[n]:
            newnode.add_interface(Interface(j[0], str(j[1]) + "/30"))
        for r in ns.rules[n]:
            newnode.add_static_route(Route(str(r[0]), str(r[1])))
        config.add_node(newnode)

    for link in ns.links:
        config.add_link(Link(link[0], link[1], link[2], link[3]))

    # Add invariants
    num_invs = 0
    random.shuffle(leaves_behind_firewalls)
    random.shuffle(leaves_wo_firewalls)
    for u in leaves_wo_firewalls:
        for v in leaves_behind_firewalls:
            if u == v:
                continue
            if not bfs_is_connected(ns.G, u, v):
                continue
            if u in emulated_node_names or v in emulated_node_names:
                continue

            dst = str(ns.node_to_interfaces[v][0][1])
            sport, dport, action = fwdata.get_random_rule()
            config.add_invariant(
                Reachability(
                    target_node=v,
                    reachable=(action == "allow"),
                    protocol="udp",
                    src_node=u,
                    dst_ip=dst,
                    src_port=sport,
                    dst_port=[dport],
                )
            )
            num_invs += 1
            if num_invs >= max_invs:
                break
        if num_invs >= max_invs:
            break

    config.output_toml()


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument(
        "-t", "--topo", type=str, help="Topology file name", required=True
    )
    parser.add_argument(
        "-e", "--emulated", type=int, help="Percentage of emulated nodes", required=True
    )
    parser.add_argument(
        "-f",
        "--firewalls",
        type=int,
        help="Percentage of leaves behind a firewall",
        required=True,
    )
    parser.add_argument(
        "-i", "--invs", type=int, help="Maximum number of invariants", required=True
    )
    arg = parser.parse_args()

    base_dir = os.path.abspath(os.path.dirname(__file__))
    in_filename = os.path.join(base_dir, "data", arg.topo)
    confgen(in_filename, arg.emulated, arg.firewalls, arg.invs)


if __name__ == "__main__":
    main()
