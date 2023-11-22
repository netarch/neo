#!/usr/bin/env python3

import logging
import os
import re
import sys
import argparse

sys.path.append(os.path.join(os.path.dirname(__file__), '../../src'))
from dataparse import *


def parse_snap(in_dir, out_dir):
    prefix = os.path.basename(in_dir)

    for entry in os.scandir(in_dir):
        if not entry.name.endswith('.txt'):
            continue

        nodes = set()
        edges = set()
        logging.info("Processing %s", entry.path)

        with open(entry.path, 'r') as f:
            for line in f:
                tokens = line.split()
                if tokens[0] == '#':
                    continue
                if tokens[0] == tokens[1]:  # self-to-self link
                    continue
                nodes.add(tokens[0])
                nodes.add(tokens[1])
                if (tokens[1], tokens[0]) not in edges:
                    edges.add((tokens[0], tokens[1]))

        out_fn = os.path.join(
            out_dir, '{}.{}-nodes.{}-edges.txt'.format(prefix, len(nodes),
                                                       len(edges)))
        with open(out_fn, 'w') as f:
            for edge in edges:
                f.write(edge[0] + ' ' + edge[1] + '\n')


def parse_rocketfuel_bb(in_dir, out_dir):
    for entry in os.scandir(in_dir):
        if not entry.is_dir():
            continue

        asn = int(entry.name)
        weight_fn = os.path.join(entry.path, 'weights.intra')
        nodes = set()
        edges = set()
        logging.info("Processing %s", weight_fn)

        with open(weight_fn, 'r') as f:
            for line in f:
                tokens = line.split()
                if tokens[0] == tokens[1]:  # self-to-self link
                    continue
                nodes.add(tokens[0])
                nodes.add(tokens[1])
                if (tokens[1], tokens[0]) not in edges:
                    edges.add((tokens[0], tokens[1]))

        out_fn = os.path.join(
            out_dir, 'rocketfuel-bb.AS-{}.{}-nodes.{}-edges.txt'.format(
                asn, len(nodes), len(edges)))
        with open(out_fn, 'w') as f:
            for edge in edges:
                f.write(edge[0] + ' ' + edge[1] + '\n')


def parse_rocketfuel_cch(in_dir, out_dir):
    for entry in os.scandir(in_dir):
        m = re.search('^(\\d+)\\.cch$', entry.name)
        if m == None:
            continue

        asn = int(m.group(1))
        nodes = set()
        edges = set()
        logging.info("Processing %s", entry.path)

        with open(entry.path, 'r') as f:
            for line in f:
                tokens = line.split()
                if tokens[0].startswith('-'):  # external nodes
                    continue
                if tokens[-1] not in ['r0', 'r1']:  # only include r0, r1 links
                    continue

                uid = tokens[0]
                i = 0
                for token in tokens:
                    i += 1
                    if token == '->':
                        break
                while (tokens[i].startswith('<') and tokens[i].endswith('>')):
                    nuid = tokens[i][1:-1]
                    i += 1

                    if (uid == nuid):  # self-to-self link
                        continue
                    nodes.add(uid)
                    nodes.add(nuid)
                    if (nuid, uid) not in edges:
                        edges.add((uid, nuid))

        out_fn = os.path.join(
            out_dir, 'rocketfuel-cch.AS-{}.{}-nodes.{}-edges.txt'.format(
                asn, len(nodes), len(edges)))
        with open(out_fn, 'w') as f:
            for edge in edges:
                f.write(edge[0] + ' ' + edge[1] + '\n')


def main():
    parser = argparse.ArgumentParser(description='Parse network datasets')
    parser.add_argument('-i',
                        '--in-dir',
                        help='Input dataset directory',
                        type=str,
                        action='store',
                        required=True)
    parser.add_argument(
        '-t',
        '--type',
        help='Dataset type',
        type=str,
        action='store',
        choices=['stanford', 'rocketfuel-bb', 'rocketfuel-cch'],
        required=True)
    arg = parser.parse_args()

    logging.basicConfig(level=logging.INFO,
                        format='[%(levelname)s] %(message)s')

    in_dir = os.path.abspath(arg.in_dir)
    if not os.path.isdir(in_dir):
        raise Exception("'" + in_dir + "' is not a directory")
    base_dir = os.path.abspath(os.path.dirname(__file__))
    out_dir = os.path.join(base_dir, 'data')
    os.makedirs(out_dir, exist_ok=True)

    if arg.type == 'stanford':
        parse_snap(in_dir, out_dir)
    elif arg.type == 'rocketfuel-bb':
        parse_rocketfuel_bb(in_dir, out_dir)
    elif arg.type == 'rocketfuel-cch':
        parse_rocketfuel_cch(in_dir, out_dir)
    else:
        raise Exception('Unknown dataset type: ' + arg.type)


if __name__ == '__main__':
    main()
