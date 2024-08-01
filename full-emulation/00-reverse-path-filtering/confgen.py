#!/usr/bin/python3

import argparse
import os
import sys

sys.path.append(os.path.join(os.path.dirname(__file__), "../.."))
from src.config import Config


def main():
    parser = argparse.ArgumentParser(description="00-reverse-path-filtering")
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
    args = parser.parse_args()

    lab_name = "reverse.path.filtering"
    config = Config()
    config.deserialize_toml(args.network)

    if args.bridges:
        for br in config.get_bridges():
            print(br)
    elif args.devices:
        bridges = config.get_bridges()
        for node in config.network.nodes:
            if node.name not in bridges:
                print(node.name)
    else:
        config.output_clab_yml(name=lab_name)


if __name__ == "__main__":
    main()
