#!/usr/bin/env python3

import argparse
import os
import re
import logging
import pandas as pd
from itertools import islice


def parse_main_log(output_dir, settings):
    settings['num_nodes'] = None
    settings['num_links'] = None
    settings['num_updates'] = 0
    settings['total_conn'] = 0
    settings['invariant'] = []
    settings['independent_cec'] = []
    settings['violated'] = []
    settings['total_time'] = None
    settings['total_mem'] = None

    mainlogFn = os.path.join(output_dir, 'main.log')
    with open(mainlogFn) as mainlog:
        inv_id = 0
        for line in mainlog:
            if re.search('Loaded (\d+) nodes', line):
                m = re.search('Loaded (\d+) nodes', line)
                settings['num_nodes'] = int(m.group(1))
            elif ' links' in line:
                m = re.search('Loaded (\d+) links', line)
                settings['num_links'] = int(m.group(1))
            elif 'openflow updates' in line:
                m = re.search('Loaded (\d+) openflow updates', line)
                settings['num_updates'] = int(m.group(1))
            elif 'Initial ECs:' in line:
                m = re.search('Initial ECs: (\d+)', line)
                settings['total_conn'] = int(m.group(1))
            elif 'Initial ports:' in line:
                m = re.search('Initial ports: (\d+)', line)
                settings['total_conn'] *= int(m.group(1))
            elif 'Verifying invariant ' in line:
                m = re.search('(\d+)\.?\s*Verifying invariant ', line)
                inv_id = int(m.group(1))
                settings['invariant'].append(inv_id)
                settings['violated'].append(False)
                # assert (len(settings['invariant']) == inv_id)
            elif 'Connection ECs:' in line:
                m = re.search('Connection ECs: (\d+)', line)
                settings['independent_cec'].append(int(m.group(1)))
                # assert (len(settings['independent_cec']) == inv_id)
            elif 'Invariant violated' in line:
                settings['violated'][inv_id - 1] = True
            elif 'Time:' in line:
                m = re.search('Time: (\d+) usec', line)
                settings['total_time'] = int(m.group(1))
            elif 'Peak memory:' in line:
                m = re.search('Peak memory: (\d+) KiB', line)
                settings['total_mem'] = int(m.group(1))


def parse_02_settings(output_dir):
    settings = {
        'apps': 0,
        'hosts': 0,
        'procs': 0,
        'drop_method': '',
        'fault': False,
    }
    dirname = os.path.basename(output_dir)
    m = re.search(
        'output\.(\d+)-apps\.(\d+)-hosts\.(\d+)-procs\.([a-z]+)(\.fault)?',
        dirname)
    settings['apps'] = int(m.group(1))
    settings['hosts'] = int(m.group(2))
    settings['procs'] = int(m.group(3))
    settings['drop_method'] = m.group(4)
    settings['fault'] = m.group(5) is not None
    parse_main_log(output_dir, settings)
    return settings


def parse_03_settings(output_dir):
    settings = {
        'lbs': 0,
        'servers': 0,
        'algorithm': '',
        'procs': 0,
        'drop_method': '',
    }
    dirname = os.path.basename(output_dir)
    m = re.search(
        'output\.(\d+)-lbs\.(\d+)-servers\.algo-([a-z]+)\.(\d+)-procs\.([a-z]+)',
        dirname)
    settings['lbs'] = int(m.group(1))
    settings['servers'] = int(m.group(2))
    settings['algorithm'] = m.group(3)
    settings['procs'] = int(m.group(4))
    settings['drop_method'] = m.group(5)
    parse_main_log(output_dir, settings)
    return settings


def parse_06_settings(output_dir):
    settings = {
        'tenants': 0,
        'updates': 0,
        'procs': 0,
        'drop_method': '',
    }
    dirname = os.path.basename(output_dir)
    m = re.search(
        'output\.(\d+)-tenants\.(\d+)-updates\.(\d+)-procs\.([a-z]+)', dirname)
    settings['tenants'] = int(m.group(1))
    settings['updates'] = int(m.group(2))
    settings['procs'] = int(m.group(3))
    settings['drop_method'] = m.group(4)
    parse_main_log(output_dir, settings)
    return settings


def parse_15_settings(output_dir):
    settings = {
        'network': '',
        'emulated_pct': 0,
        'invariants': 0,
        'procs': 0,
        'drop_method': '',
    }
    dirname = os.path.basename(output_dir)
    m = re.search(
        'output\.([^\.]+)\.\d+-nodes\.\d+-edges\.(\d+)-emulated\.(\d+)-invariants\.(\d+)-procs\.([a-z]+)',
        dirname)
    settings['network'] = m.group(1)
    settings['emulated_pct'] = int(m.group(2))
    settings['invariants'] = int(m.group(3))
    settings['procs'] = int(m.group(4))
    settings['drop_method'] = m.group(5)
    parse_main_log(output_dir, settings)
    return settings


def parse_18_settings(output_dir):
    settings = {
        'arity': 0,
        'update_pct': 0,
        'procs': 0,
        'drop_method': '',
    }
    dirname = os.path.basename(output_dir)
    m = re.search('output\.(\d+)-ary\.(\d+)-update-pct\.(\d+)-procs\.([a-z]+)',
                  dirname)
    settings['arity'] = int(m.group(1))
    settings['update_pct'] = int(m.group(2))
    settings['procs'] = int(m.group(3))
    settings['drop_method'] = m.group(4)
    parse_main_log(output_dir, settings)
    return settings


def parse_stats(inv_dir, settings, stats):
    invStatsFn = os.path.join(inv_dir, 'invariant.stats.csv')
    with open(invStatsFn) as inv_stats:
        tokens = next(islice(inv_stats, 1, 2)).strip().split(',')
        stats['inv_time'].append(int(tokens[0]))
        stats['inv_memory'].append(int(tokens[1]))

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
        if (not entry.name.endswith('stats.csv')
                or entry.name.startswith('invariant')):
            continue

        ec_time = None
        ec_mem = None
        with open(entry.path) as ec_lat_file:
            tokens = next(islice(ec_lat_file, 1, 2)).strip().split(',')
            ec_time = int(tokens[0])
            ec_mem = int(tokens[1])

        with open(entry.path) as ec_lat_file:
            for line in islice(ec_lat_file, 3, None):
                tokens = line.split(',')
                latencies['ec_time'].append(ec_time)
                latencies['ec_mem'].append(ec_mem)
                latencies['overall_lat'].append(int(tokens[0]))
                latencies['emu_startup'].append(int(tokens[1]))
                latencies['rewind'].append(int(tokens[2]))
                latencies['emu_reset'].append(int(tokens[3]))
                latencies['replay'].append(int(tokens[4]))
                latencies['rewind_injections'].append(int(tokens[5]))
                latencies['pkt_lat'].append(int(tokens[6]))
                latencies['drop_lat'].append(int(tokens[7]))
                latencies['timeout'].append(int(tokens[8]))
                num_injections += 1

    inv_id = int(os.path.basename(inv_dir))
    for key in settings.keys():
        if key not in latencies:
            latencies[key] = list()
        if type(settings[key]) is list:
            latencies[key].extend([settings[key][inv_id - 1]] * num_injections)
        else:
            latencies[key].extend([settings[key]] * num_injections)


def parse(base_dir):
    settings = {}
    stats = {
        'inv_time': [],  # usec
        'inv_memory': [],  # KiB
    }
    latencies = {
        'ec_time': [],  # usec
        'ec_mem': [],  # KiB
        'overall_lat': [],  # usec
        'emu_startup': [],  # usec
        'rewind': [],  # usec
        'emu_reset': [],  # usec
        'replay': [],  # usec
        'rewind_injections': [],
        'pkt_lat': [],  # usec
        'drop_lat': [],  # usec
        'timeout': [],  # usec
    }
    exp_name = os.path.basename(base_dir)
    exp_id = exp_name[:2]
    results_dir = os.path.join(base_dir, 'results')

    for entry in os.scandir(results_dir):
        if not entry.is_dir():
            continue

        logging.info("Processing %s -- %s", exp_name, entry.name)
        output_dir = entry.path

        if exp_id == '02':
            settings = parse_02_settings(output_dir)
        elif exp_id == '03':
            settings = parse_03_settings(output_dir)
        elif exp_id == '06':
            settings = parse_06_settings(output_dir)
        elif exp_id == '15':
            settings = parse_15_settings(output_dir)
        elif exp_id == '18':
            settings = parse_18_settings(output_dir)
        else:
            raise Exception("Parser not implemented for experiment " + exp_id)

        for entry in os.scandir(output_dir):
            if not entry.is_dir() or not entry.name.isdigit():
                continue

            parse_stats(entry.path, settings, stats)
            parse_latencies(entry.path, settings, latencies)

    stats_df = pd.DataFrame.from_dict(stats)
    lat_df = pd.DataFrame.from_dict(latencies)
    stats_df.to_csv(os.path.join(base_dir, 'stats.csv'),
                    encoding='utf-8',
                    index=False)
    lat_df.to_csv(os.path.join(base_dir, 'lat.csv'),
                  encoding='utf-8',
                  index=False)


def merge_model(model_stats_fn, neo_stats_fn):
    model_stats_df = pd.read_csv(model_stats_fn)
    neo_stats_df = pd.read_csv(neo_stats_fn)
    model_stats_df['model_only'] = True
    neo_stats_df['model_only'] = False
    stats_df = pd.concat([model_stats_df, neo_stats_df], ignore_index=True)
    stats_df.to_csv('stats.csv', encoding='utf-8', index=False)


def merge_unoptimized(opted_stats_fn, unopt_stats_fn, opted_lat_fn,
                      unopt_lat_fn):
    opted_stats = pd.read_csv(opted_stats_fn)
    unopt_stats = pd.read_csv(unopt_stats_fn)
    opted_stats['optimization'] = True
    unopt_stats['optimization'] = False
    df = pd.concat([opted_stats, unopt_stats], ignore_index=True)
    df.to_csv('stats.csv', encoding='utf-8', index=False)

    opted_lat = pd.read_csv(opted_lat_fn)
    unopt_lat = pd.read_csv(unopt_lat_fn)
    opted_lat['optimization'] = True
    unopt_lat['optimization'] = False
    df = pd.concat([opted_lat, unopt_lat], ignore_index=True)
    df.to_csv('lat.csv', encoding='utf-8', index=False)


def main():
    parser = argparse.ArgumentParser(description='Log parser for Neo')
    parser.add_argument('-t',
                        '--target',
                        help='Parser target',
                        type=str,
                        action='store')
    parser.add_argument('--merge-model',
                        help='Merge the model-based and neo stats',
                        action='store_true')
    parser.add_argument('--model-stats',
                        help='Model-based stats csv file',
                        type=str,
                        action='store')
    parser.add_argument('--neo-stats',
                        help='Neo stats csv file',
                        type=str,
                        action='store')
    parser.add_argument('--merge-unopt',
                        help='Merge unoptimized and optimized stats',
                        action='store_true')
    parser.add_argument('--opted-stats',
                        help='Optimized (original) stats csv',
                        type=str,
                        action='store')
    parser.add_argument('--unopt-stats',
                        help='Unoptimized stats csv',
                        type=str,
                        action='store')
    parser.add_argument('--opted-lat',
                        help='Optimized (original) lat csv',
                        type=str,
                        action='store')
    parser.add_argument('--unopt-lat',
                        help='Unoptimized lat csv',
                        type=str,
                        action='store')
    args = parser.parse_args()

    logging.basicConfig(level=logging.INFO,
                        format='[%(levelname)s] %(message)s')

    if args.merge_model:
        merge_model(args.model_stats, args.neo_stats)
    elif args.merge_unopt:
        merge_unoptimized(args.opted_stats, args.unopt_stats, args.opted_lat,
                          args.unopt_lat)
    else:
        target_dir = os.path.abspath(args.target)
        if not os.path.isdir(target_dir):
            raise Exception("'" + target_dir + "' is not a directory")

        exp_id = os.path.basename(target_dir)[:2]
        if exp_id in ['02', '03', '06', '15', '18']:
            parse(target_dir)
        else:
            raise Exception("Parser not implemented for experiment " + exp_id)


if __name__ == '__main__':
    main()
