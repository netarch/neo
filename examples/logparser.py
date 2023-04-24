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
                assert (len(settings['invariant']) == inv_id)
            elif 'Connection ECs:' in line:
                m = re.search('Connection ECs: (\d+)', line)
                settings['independent_cec'].append(int(m.group(1)))
                assert (len(settings['independent_cec']) == inv_id)
            elif 'Invariant violated' in line:
                settings['violated'][inv_id - 1] = True


def parse_02_settings(output_dir):
    settings = {
        'apps': None,
        'hosts': None,
        'procs': None,
        'drop_method': None,
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
        'lbs': None,
        'servers': None,
        'algorithm': None,
        'procs': None,
        'drop_method': None,
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
        'tenants': None,
        'updates': None,
        'procs': None,
        'drop_method': None,
    }
    dirname = os.path.basename(output_dir)
    m = re.search(
        'output\.(\d+)-tenants\.(\d+)-updates\.(\d+)-procs\.([a-z]+)',
        dirname)
    settings['tenants'] = int(m.group(1))
    settings['updates'] = int(m.group(2))
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

        with open(entry.path) as ec_lat_file:
            for line in islice(ec_lat_file, 3, None):
                tokens = line.split(',')
                latencies['overall_lat'].append(int(tokens[0]))
                latencies['rewind_lat'].append(int(tokens[1]))
                latencies['rewind_injections'].append(int(tokens[2]))
                latencies['pkt_lat'].append(int(tokens[3]))
                latencies['drop_lat'].append(int(tokens[4]))
                latencies['timeout'].append(int(tokens[5]))
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
        # usec
        'overall_lat': [],
        'rewind_lat': [],
        'rewind_injections': [],
        'pkt_lat': [],
        'drop_lat': [],
        'timeout': [],
    }
    exp_id = os.path.basename(base_dir)[:2]
    results_dir = os.path.join(base_dir, 'results')

    for entry in os.scandir(results_dir):
        if not entry.is_dir():
            continue

        logging.info("Processing %s", entry.name)
        output_dir = entry.path

        if exp_id == '02':
            settings = parse_02_settings(output_dir)
        elif exp_id == '03':
            settings = parse_03_settings(output_dir)
        elif exp_id == '06':
            settings = parse_06_settings(output_dir)
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


def merge_stats(model_stats_fn, neo_stats_fn):
    model_stats_df = pd.read_csv(model_stats_fn)
    neo_stats_df = pd.read_csv(neo_stats_fn)
    model_stats_df['model_only'] = True
    neo_stats_df['model_only'] = False
    stats_df = pd.concat([model_stats_df, neo_stats_df], ignore_index=True)
    stats_df.to_csv('stats.csv', encoding='utf-8', index=False)


def main():
    parser = argparse.ArgumentParser(description='Log parser for Neo')
    parser.add_argument('-t',
                        '--target',
                        help='Parser target',
                        type=str,
                        action='store')
    parser.add_argument('--merge',
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
    args = parser.parse_args()

    logging.basicConfig(level=logging.INFO,
                        format='[%(levelname)s] %(message)s')

    if args.merge:
        merge_stats(args.model_stats, args.neo_stats)
    else:
        target_dir = os.path.abspath(args.target)
        if not os.path.isdir(target_dir):
            raise Exception("'" + target_dir + "' is not a directory")

        exp_id = os.path.basename(target_dir)[:2]
        if exp_id in ['02', '03', '06']:
            parse(target_dir)
        else:
            raise Exception("Parser not implemented for experiment " + exp_id)


if __name__ == '__main__':
    main()
