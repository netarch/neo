#!/usr/bin/python3

import argparse
import os
import sys

sys.path.append(os.path.join(os.path.dirname(__file__), "../.."))
from src.config import Config


def main():
    parser = argparse.ArgumentParser(description="17-campus-network")
    parser.add_argument(
        "-n",
        "--network",
        type=str,
        required=True,
        action="store",
        help="Input network toml file",
    )
    parser.add_argument(
        "-b",
        "--bridges",
        default=False,
        action="store_true",
        help="Print out all L2 bridge names",
    )
    parser.add_argument(
        "-d",
        "--devices",
        default=False,
        action="store_true",
        help="Print out all device names except bridges",
    )
    parser.add_argument(
        "-i",
        "--invariants",
        default=False,
        action="store_true",
        help="Print out invariant parameters",
    )
    args = parser.parse_args()

    lab_name = "campus_network"
    config = Config()
    config.deserialize_toml(args.network)
    config.prefix_all_node_names("node_")  # Prefix all node names with "node_"
    config.mangle_l2_switch_intf_names()

    if args.bridges:
        for br in config.get_bridges():
            print(br)
    elif args.devices:
        bridges = config.get_bridges()
        for node in config.network.nodes:
            if node.name not in bridges:
                print(node.name)
    elif args.invariants:
        config.output_invariants()
    else:
        config.output_clab_yml(name=lab_name)


if __name__ == "__main__":
    main()
