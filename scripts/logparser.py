#!/usr/bin/env python3

import argparse
import logging
import os
import re
from collections import deque
from itertools import islice

import pandas as pd


def gnu_time_str_to_usec(time_str: str) -> int:
    if re.match(r"(\d+):(\d+)\.(\d+)", time_str):
        m = re.match(r"(\d+):(\d+)\.(\d+)", time_str)
        assert m is not None
        min = int(m.group(1))
        sec = int(m.group(2))
        subsec = str(m.group(3))
        assert (6 - len(subsec)) >= 0
        usec = int(subsec) * (10 ** (6 - len(subsec)))
        usec += (min * 60 + sec) * (10**6)
    elif re.match(r"(\d+):(\d+):(\d+)", time_str):
        m = re.match(r"(\d+):(\d+):(\d+)", time_str)
        assert m is not None
        hour = int(m.group(1))
        min = int(m.group(2))
        sec = int(m.group(3))
        usec = ((hour * 60 + min) * 60 + sec) * (10**6)
    else:
        raise Exception("Unknown GNU time: '" + time_str + "'")
    return usec


def parse_main_output(output_dir, settings):
    settings["num_nodes"] = None
    settings["num_links"] = None
    settings["num_updates"] = 0
    settings["total_conn"] = 0
    settings["invariant"] = []
    settings["independent_cec"] = []
    settings["violated"] = []
    settings["total_time"] = None
    settings["total_mem"] = None

    outlogFn = os.path.join(output_dir, "out.log")
    with open(outlogFn) as outlog:
        inv_id = 0
        for line in outlog:
            if re.search(r"Loaded (\d+) nodes", line):
                m = re.search(r"Loaded (\d+) nodes", line)
                assert m is not None
                settings["num_nodes"] = int(m.group(1))
            elif " links" in line:
                m = re.search(r"Loaded (\d+) links", line)
                assert m is not None
                settings["num_links"] = int(m.group(1))
            elif "openflow updates" in line:
                m = re.search(r"Loaded (\d+) openflow updates", line)
                assert m is not None
                settings["num_updates"] = int(m.group(1))
            elif "Initial ECs:" in line:
                m = re.search(r"Initial ECs: (\d+)", line)
                assert m is not None
                settings["total_conn"] = int(m.group(1))
            elif "Initial ports:" in line:
                m = re.search(r"Initial ports: (\d+)", line)
                assert m is not None
                settings["total_conn"] *= int(m.group(1))
            elif "Verifying invariant " in line:
                m = re.search(r"(\d+)\.?\s*Verifying invariant ", line)
                assert m is not None
                inv_id = int(m.group(1))
                settings["invariant"].append(inv_id)
                settings["violated"].append(False)
                # assert (len(settings['invariant']) == inv_id)
            elif "Connection ECs:" in line:
                m = re.search(r"Connection ECs: (\d+)", line)
                assert m is not None
                settings["independent_cec"].append(int(m.group(1)))
                # assert (len(settings['independent_cec']) == inv_id)
            elif "Invariant violated" in line:
                settings["violated"][inv_id - 1] = True
            elif "Time:" in line:
                m = re.search(r"Time: (\d+) usec", line)
                assert m is not None
                settings["total_time"] = int(m.group(1))
            elif "maxresident" in line:
                m = re.search(r" (\d+)maxresident", line)
                assert m is not None
                settings["total_mem"] = int(m.group(1))


# def parse_02_settings(output_dir):
#     settings = {
#         "apps": 0,
#         "hosts": 0,
#         "procs": 0,
#         "drop_method": "",
#         "fault": False,
#     }
#     dirname = os.path.basename(output_dir)
#     m = re.search(
#         r"output\.(\d+)-apps\.(\d+)-hosts\.(\d+)-procs\.([a-z]+)(\.fault)?", dirname
#     )
#     assert m is not None
#     settings["apps"] = int(m.group(1))
#     settings["hosts"] = int(m.group(2))
#     settings["procs"] = int(m.group(3))
#     settings["drop_method"] = m.group(4)
#     settings["fault"] = m.group(5) is not None
#     parse_main_output(output_dir, settings)
#     return settings


def parse_03_settings(output_dir):
    settings = {
        "lbs": 0,
        "servers": 0,
        "algorithm": "",
        "procs": 0,
        "drop_method": "",
    }
    dirname = os.path.basename(output_dir)
    m = re.search(
        r"output\.(\d+)-lbs\.(\d+)-servers\.algo-([a-z]+)\.(\d+)-procs\.([a-z]+)",
        dirname,
    )
    assert m is not None
    settings["lbs"] = int(m.group(1))
    settings["servers"] = int(m.group(2))
    settings["algorithm"] = m.group(3)
    settings["procs"] = int(m.group(4))
    settings["drop_method"] = m.group(5)
    parse_main_output(output_dir, settings)
    return settings


# def parse_06_settings(output_dir):
#     settings = {
#         "tenants": 0,
#         "updates": 0,
#         "procs": 0,
#         "drop_method": "",
#     }
#     dirname = os.path.basename(output_dir)
#     m = re.search(
#         r"output\.(\d+)-tenants\.(\d+)-updates\.(\d+)-procs\.([a-z]+)", dirname
#     )
#     assert m is not None
#     settings["tenants"] = int(m.group(1))
#     settings["updates"] = int(m.group(2))
#     settings["procs"] = int(m.group(3))
#     settings["drop_method"] = m.group(4)
#     parse_main_output(output_dir, settings)
#     return settings


def parse_15_settings(output_dir):
    settings = {
        "network": "",
        "emulated_pct": 0,
        "fw_leaves_pct": 0,
        "invariants": 0,
        "procs": 0,
        "drop_method": "",
    }
    dirname = os.path.basename(output_dir)
    m = re.search(
        r"output\.([^\.]+)\.\d+-nodes\.\d+-edges\.(\d+)-emulated\.(\d+)-fwleaves\.(\d+)-invariants\.(\d+)-procs\.([a-z]+)",
        dirname,
    )
    assert m is not None
    settings["network"] = m.group(1)
    settings["emulated_pct"] = int(m.group(2))
    settings["fw_leaves_pct"] = int(m.group(3))
    settings["invariants"] = int(m.group(4))
    settings["procs"] = int(m.group(5))
    settings["drop_method"] = m.group(6)
    parse_main_output(output_dir, settings)
    return settings


def parse_17_settings(output_dir) -> dict:
    settings = {
        "network": "",
        "procs": 0,
        "drop_method": "",
    }
    dirname = os.path.basename(output_dir)
    m = re.search(r"output\.([^\.]+)\.(\d+)-procs\.([a-z]+)", dirname)
    assert m is not None
    settings["network"] = m.group(1)
    settings["procs"] = int(m.group(2))
    settings["drop_method"] = m.group(3)
    parse_main_output(output_dir, settings)
    return settings


def parse_18_settings(output_dir):
    settings = {
        "arity": 0,
        "update_pct": 0,
        "procs": 0,
        "drop_method": "",
    }
    dirname = os.path.basename(output_dir)
    m = re.search(
        r"output\.(\d+)-ary\.(\d+)-update-pct\.(\d+)-procs\.([a-z]+)", dirname
    )
    assert m is not None
    settings["arity"] = int(m.group(1))
    settings["update_pct"] = int(m.group(2))
    settings["procs"] = int(m.group(3))
    settings["drop_method"] = m.group(4)
    parse_main_output(output_dir, settings)
    return settings


def parse_stats(inv_dir, settings, stats):
    invStatsFn = os.path.join(inv_dir, "invariant.stats.csv")
    with open(invStatsFn) as inv_stats:
        tokens = next(islice(inv_stats, 1, 2)).strip().split(",")
        stats["inv_time"].append(int(tokens[0]))
        stats["inv_memory"].append(int(tokens[1]))

    inv_id = int(os.path.basename(inv_dir))
    for key in settings.keys():
        if key not in stats:
            stats[key] = list()
        if type(settings[key]) is list:
            stats[key].append(settings[key][inv_id - 1])
        else:
            stats[key].append(settings[key])


def parse_latencies(inv_dir, settings, latencies):
    num_injections = 0

    for entry in os.scandir(inv_dir):
        if not entry.name.endswith("stats.csv") or entry.name.startswith("invariant"):
            continue

        ec_time = None
        ec_mem = None
        with open(entry.path) as ec_lat_file:
            tokens = next(islice(ec_lat_file, 1, 2)).strip().split(",")
            ec_time = int(tokens[0])
            ec_mem = int(tokens[1])

        with open(entry.path) as ec_lat_file:
            for line in islice(ec_lat_file, 3, None):
                tokens = line.split(",")
                latencies["ec_time"].append(ec_time)
                latencies["ec_mem"].append(ec_mem)
                latencies["overall_lat"].append(int(tokens[0]))
                latencies["emu_startup"].append(int(tokens[1]))
                latencies["rewind"].append(int(tokens[2]))
                latencies["emu_reset"].append(int(tokens[3]))
                latencies["replay"].append(int(tokens[4]))
                latencies["rewind_injections"].append(int(tokens[5]))
                latencies["pkt_lat"].append(int(tokens[6]))
                latencies["drop_lat"].append(int(tokens[7]))
                latencies["timeout"].append(int(tokens[8]))
                num_injections += 1

    inv_id = int(os.path.basename(inv_dir))
    for key in settings.keys():
        if key not in latencies:
            latencies[key] = list()
        if type(settings[key]) is list:
            latencies[key].extend([settings[key][inv_id - 1]] * num_injections)
        else:
            latencies[key].extend([settings[key]] * num_injections)


def parse_neo_results(base_dir):
    settings = {}
    stats = {
        "inv_time": [],  # usec
        "inv_memory": [],  # KiB
    }
    latencies = {
        "ec_time": [],  # usec
        "ec_mem": [],  # KiB
        "overall_lat": [],  # usec
        "emu_startup": [],  # usec
        "rewind": [],  # usec
        "emu_reset": [],  # usec
        "replay": [],  # usec
        "rewind_injections": [],
        "pkt_lat": [],  # usec
        "drop_lat": [],  # usec
        "timeout": [],  # usec
    }
    exp_name = os.path.basename(base_dir)
    exp_id = exp_name[:2]
    results_dir = os.path.join(base_dir, "results")

    for entry in os.scandir(results_dir):
        if not entry.is_dir():
            continue
        logging.info("Processing %s -- %s", exp_name, entry.name)
        output_dir = entry.path

        if exp_id == "03":
            settings = parse_03_settings(output_dir)
        elif exp_id == "15":
            settings = parse_15_settings(output_dir)
        elif exp_id == "17":
            settings = parse_17_settings(output_dir)
        elif exp_id == "18":
            settings = parse_18_settings(output_dir)
        else:
            raise Exception("Parser not implemented for {}".format(exp_id))

        for entry in os.scandir(output_dir):
            if not entry.is_dir() or not entry.name.isdigit():
                continue
            parse_stats(entry.path, settings, stats)
            parse_latencies(entry.path, settings, latencies)

    stats_df = pd.DataFrame.from_dict(stats)
    lat_df = pd.DataFrame.from_dict(latencies)
    stats_df.to_csv(os.path.join(base_dir, "stats.csv"), encoding="utf-8", index=False)
    lat_df.to_csv(os.path.join(base_dir, "lat.csv"), encoding="utf-8", index=False)


def parse_emu_00(base_dir: str, results_dir: str) -> None:
    stats = {
        "run_id": [],
        "packets_received": [],
        "packets_total": [],
        "container_memory": [],  # KB
        "total_time": [],  # usec
        "total_mem": [],  # KB
    }
    for entry in os.scandir(results_dir):
        if not entry.is_file() or not entry.name.endswith(".log"):
            continue
        m = re.match(r"(\d+)\.log", entry.name)
        assert m is not None
        run_id = int(m.group(1))
        pkts_recvd = 0
        pkts_total = 0
        cntr_mem = None
        total_time = None
        total_mem = None
        with open(entry.path) as fin:
            for line in fin:
                if "packets received" in line:
                    m = re.search(
                        r"(\d+) packets tr.*mitted, (\d+) packets received", line
                    )
                    assert m is not None
                    pkts_total += int(m.group(1))
                    pkts_recvd += int(m.group(2))
                elif "Total container memory" in line:
                    m = re.search(r"Total container memory: (\d+) KB", line)
                    assert m is not None
                    cntr_mem = int(m.group(1))
                elif "maxresident" in line:
                    m = re.search(r"([0-9:.]+)elapsed .* (\d+)maxresident", line)
                    assert m is not None
                    total_time = gnu_time_str_to_usec(str(m.group(1)))
                    total_mem = int(m.group(2))
        stats["run_id"].append(run_id)
        stats["packets_received"].append(pkts_recvd)
        stats["packets_total"].append(pkts_total)
        stats["container_memory"].append(cntr_mem)
        stats["total_time"].append(total_time)
        stats["total_mem"].append(total_mem)

    stats_df = pd.DataFrame.from_dict(stats)
    stats_df = stats_df.sort_values(by=["run_id"])
    stats_df.to_csv(os.path.join(base_dir, "stats.csv"), encoding="utf-8", index=False)


def parse_emu_15_settings(network_dir: str) -> dict:
    settings = {"network": ""}
    network_str = os.path.basename(network_dir)
    m = re.search(r"([^\.]+)\.\d+-nodes\.\d+-edges", network_str)
    assert m is not None
    settings["network"] = m.group(1)
    return settings


def parse_emu_17_settings(network_dir: str) -> dict:
    settings = {"network": ""}
    network_str = os.path.basename(network_dir)
    m = re.search(r"network-([^\.]+)", network_str)
    assert m is not None
    settings["network"] = m.group(1)
    return settings


def parse_emu_stats(network_dir: str, settings: dict, stats: dict) -> None:
    logfn = os.path.join(network_dir, "out.log")
    with open(logfn) as fin:
        last3 = list(deque(fin, 3))
        for line in last3:
            if "Total container memory" in line:
                m = re.search(r"Total container memory: (\d+) KB", line)
                assert m is not None
                stats["container_memory"].append(int(m.group(1)))
            elif "maxresident" in line:
                m = re.search(r"([0-9:.]+)elapsed .* (\d+)maxresident", line)
                assert m is not None
                stats["total_time"].append(gnu_time_str_to_usec(str(m.group(1))))
                stats["total_mem"].append(int(m.group(2)))

    for key in settings.keys():
        if key not in stats:
            stats[key] = list()
        assert type(settings[key]) is str
        stats[key].append(settings[key])


def parse_emu_results(base_dir):
    exp_name = os.path.basename(base_dir)
    exp_id = exp_name[:2]
    results_dir = os.path.join(base_dir, "results")

    if exp_id == "00":
        parse_emu_00(base_dir, results_dir)
        return

    settings = {}
    stats = {
        "container_memory": [],  # KB
        "total_time": [],  # ? (usec?)
        "total_mem": [],  # KB
    }
    for entry in os.scandir(results_dir):
        if not entry.is_dir():
            continue
        logging.info("Processing %s -- %s", exp_name, entry.name)
        network_dir = entry.path

        if exp_id == "15":
            settings = parse_emu_15_settings(network_dir)
        elif exp_id == "17":
            settings = parse_emu_17_settings(network_dir)
        else:
            raise Exception("Parser not implemented for {}".format(exp_id))

        parse_emu_stats(network_dir, settings, stats)

    stats_df = pd.DataFrame.from_dict(stats)
    stats_df = stats_df.sort_values(by=["network", "total_mem"])
    stats_df.to_csv(os.path.join(base_dir, "stats.csv"), encoding="utf-8", index=False)


def merge_unoptimized(opted_stats_fn, unopt_stats_fn, opted_lat_fn, unopt_lat_fn):
    opted_stats = pd.read_csv(opted_stats_fn)
    unopt_stats = pd.read_csv(unopt_stats_fn)
    opted_stats["optimization"] = True
    unopt_stats["optimization"] = False
    df = pd.concat([opted_stats, unopt_stats], ignore_index=True)
    df.to_csv("stats.csv", encoding="utf-8", index=False)

    opted_lat = pd.read_csv(opted_lat_fn)
    unopt_lat = pd.read_csv(unopt_lat_fn)
    opted_lat["optimization"] = True
    unopt_lat["optimization"] = False
    df = pd.concat([opted_lat, unopt_lat], ignore_index=True)
    df.to_csv("lat.csv", encoding="utf-8", index=False)


def main():
    parser = argparse.ArgumentParser(description="Log parser for Neo")
    parser.add_argument(
        "-t", "--target", help="Parser target", type=str, action="store"
    )
    parser.add_argument(
        "--merge-unopt",
        help="Merge unoptimized and optimized stats",
        action="store_true",
    )
    parser.add_argument(
        "--opted-stats", help="Optimized (original) stats csv", type=str, action="store"
    )
    parser.add_argument(
        "--unopt-stats", help="Unoptimized stats csv", type=str, action="store"
    )
    parser.add_argument(
        "--opted-lat", help="Optimized (original) lat csv", type=str, action="store"
    )
    parser.add_argument(
        "--unopt-lat", help="Unoptimized lat csv", type=str, action="store"
    )
    args = parser.parse_args()

    logging.basicConfig(level=logging.INFO, format="[%(levelname)s] %(message)s")

    if args.merge_unopt:
        merge_unoptimized(
            args.opted_stats, args.unopt_stats, args.opted_lat, args.unopt_lat
        )
    else:
        target_dir = os.path.abspath(args.target)
        if not os.path.isdir(target_dir):
            raise Exception("'" + target_dir + "' is not a directory")
        category = os.path.basename(os.path.dirname(target_dir))
        if category == "examples":
            parse_neo_results(target_dir)
        elif category == "full-emulation":
            parse_emu_results(target_dir)
        else:
            raise Exception("Unknown category: {}".format(category))


if __name__ == "__main__":
    main()
