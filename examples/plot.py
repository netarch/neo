#!/usr/bin/env python3

import argparse
import os
import re
import logging
import numpy as np
import pandas as pd
from matplotlib import pyplot as plt
from math import floor

LINE_WIDTH = 3


def plot_06_perf_vs_tenants(df, outDir):

    def _plot(df, outDir, nproc, drop, inv):
        # Filter columns
        df = df.drop([
            'model_only', 'num_nodes', 'num_links', 'num_updates',
            'independent_cec', 'violated'
        ],
                     axis=1)
        # Sorting
        df = df.sort_values(by=['tenants', 'updates'])
        # Change units
        df['inv_time'] /= 1e6  # usec -> sec
        df['inv_memory'] /= 1024  # KiB -> MiB

        time_df = df.pivot(index='tenants',
                           columns='updates',
                           values='inv_time').reset_index()
        time_df = time_df.rename(
            columns={
                'None': 'Time (none)',
                'Half-tenant': 'Time (half-tenant)',
                'All-tenant': 'Time (all-tenant)',
            })
        mem_df = df.pivot(index='tenants',
                          columns='updates',
                          values='inv_memory').reset_index()
        mem_df = mem_df.rename(
            columns={
                'None': 'Memory (none)',
                'Half-tenant': 'Memory (half-tenant)',
                'All-tenant': 'Memory (all-tenant)',
            })
        merged_df = pd.merge(time_df, mem_df, on=['tenants'])

        # Plot time/memory
        ax = merged_df.plot(
            x='tenants',
            y=['Time (none)', 'Time (half-tenant)', 'Time (all-tenant)'],
            kind='bar',
            legend=False,
            width=0.8,
            xlabel='',
            ylabel='',
            rot=0,
        )
        ax = merged_df.plot(
            x='tenants',
            secondary_y=[
                'Memory (none)', 'Memory (half-tenant)', 'Memory (all-tenant)'
            ],
            mark_right=False,
            kind='bar',
            legend=False,
            width=0.8,
            xlabel='',
            ylabel='',
            rot=0,
        )

        # Merge legends
        h1, l1 = ax.get_legend_handles_labels()
        h2, l2 = ax.right_ax.get_legend_handles_labels()
        ax.legend(h1 + h2,
                  l1 + l2,
                  ncol=1,
                  fontsize=14,
                  frameon=False,
                  fancybox=False)

        ax.grid(axis='y')
        ax.set_yscale('log')
        ax.set_xlabel('Tenants', fontsize=22)
        ax.set_ylabel('Time (seconds)', fontsize=22)
        ax.tick_params(axis='both', which='both', labelsize=22)
        ax.tick_params(axis='x', which='both', top=False, bottom=False)
        ax.right_ax.grid(axis='y')
        mem_start_val = (floor(df.inv_memory.min()) - 1) // 10 * 10
        ax.right_ax.set_ylim(bottom=mem_start_val)
        ax.right_ax.set_ylabel('Memory (MiB)', fontsize=22)
        ax.right_ax.tick_params(axis='both', which='both', labelsize=18)
        fig = ax.get_figure()
        fn = os.path.join(outDir, ('06.perf-tenants.inv-' + str(inv) + '.' +
                                   str(nproc) + '-procs.' + drop + '.pdf'))
        fig.savefig(fn, bbox_inches='tight')
        plt.close('all')

    # Filter rows
    df = df[df.model_only == False]

    for nproc in df.procs.unique():
        nproc_df = df[df.procs == nproc].drop(['procs'], axis=1)

        for drop in nproc_df.drop_method.unique():
            d_df = nproc_df[nproc_df.drop_method == drop].drop(['drop_method'],
                                                               axis=1)

            for inv in d_df.invariant.unique():
                inv_df = d_df[d_df.invariant == inv].drop(['invariant'],
                                                          axis=1)
                _plot(inv_df, outDir, nproc, drop, inv)


def plot_06_perf_vs_nprocs(df, outDir):

    def _plot(df, outDir, drop, inv):
        # Filter columns
        df = df.drop([
            'inv_memory', 'model_only', 'num_nodes', 'num_links',
            'num_updates', 'independent_cec', 'violated', 'updates'
        ],
                     axis=1)
        # Sorting
        df = df.sort_values(by=['procs', 'tenants'])
        # Change units
        df['inv_time'] /= 1e6  # usec -> sec
        # Rename values
        df['tenants'] = df['tenants'].astype(str) + ' tenants'

        df = df.pivot(index='procs', columns='tenants',
                      values='inv_time').reset_index()

        # Plot time vs nprocs
        ax = df.plot(
            x='procs',
            y=[
                '4 tenants', '8 tenants', '12 tenants', '32 tenants',
                '64 tenants'
            ],
            kind='line',
            style=['^-', 'o-', 'D-', 's-', 'x-'],
            legend=False,
            xlabel='',
            ylabel='',
            rot=0,
            lw=LINE_WIDTH,
        )
        ax.legend(ncol=2, fontsize=14, frameon=False, fancybox=False)
        ax.grid(axis='y')
        # ax.set_yscale('log')
        ax.set_xlabel('Parallel processes', fontsize=22)
        ax.set_ylabel('Time (seconds)', fontsize=22)
        ax.tick_params(axis='both', which='both', labelsize=22)
        ax.tick_params(axis='x', which='both', top=False, bottom=False)
        fig = ax.get_figure()
        fn = os.path.join(
            outDir, ('06.perf-nproc.inv-' + str(inv) + '.' + drop + '.pdf'))
        fig.savefig(fn, bbox_inches='tight')
        plt.close(fig)

    # Filter rows
    df = df[df.model_only == False]
    df = df[df.updates == 'None']

    for drop in df.drop_method.unique():
        d_df = df[df.drop_method == drop].drop(['drop_method'], axis=1)

        for inv in d_df.invariant.unique():
            inv_df = d_df[d_df.invariant == inv].drop(['invariant'], axis=1)
            _plot(inv_df, outDir, drop, inv)


def plot_06_perf_vs_drop_methods(df, outDir):

    def _plot(df, outDir, updates, nproc, inv):
        # Filter columns
        df = df.drop([
            'inv_memory', 'num_nodes', 'num_links', 'num_updates',
            'independent_cec', 'violated', 'model_only'
        ],
                     axis=1)
        # Sorting
        df = df.sort_values(by=['tenants', 'drop_method'])
        # Change units
        df['inv_time'] /= 1e6  # usec -> sec
        # Rename values
        df = (df.replace('dropmon', 'drop_mon'))

        df = df.pivot(index='tenants',
                      columns='drop_method',
                      values='inv_time').reset_index()

        # Plot time w. drop methods
        ax = df.plot(
            x='tenants',
            y=['timeout', 'ebpf', 'drop_mon'],
            kind='bar',
            legend=False,
            width=0.8,
            xlabel='',
            ylabel='',
            rot=0,
        )
        ax.legend(bbox_to_anchor=(1.1, 1.18),
                  columnspacing=0.7,
                  ncol=3,
                  fontsize=20,
                  frameon=False,
                  fancybox=False)
        ax.grid(axis='y')
        # ax.set_yscale('log')
        ax.set_xlabel('Tenants', fontsize=22)
        ax.set_ylabel('Time (seconds)', fontsize=22)
        ax.tick_params(axis='both', which='both', labelsize=22)
        ax.tick_params(axis='x', which='both', top=False, bottom=False)
        fig = ax.get_figure()
        fn = os.path.join(
            outDir, ('06.perf-drop.inv-' + str(inv) + '.' + updates.lower() +
                     '-updates.' + str(nproc) + '-procs.pdf'))
        fig.savefig(fn, bbox_inches='tight')
        plt.close(fig)

    # Filter rows
    df = df[df.model_only == False]

    for updates in df.updates.unique():
        upd_df = df[df.updates == updates].drop(['updates'], axis=1)

        for nproc in upd_df.procs.unique():
            nproc_df = upd_df[upd_df.procs == nproc].drop(['procs'], axis=1)

            for inv in nproc_df.invariant.unique():
                inv_df = nproc_df[nproc_df.invariant == inv].drop(
                    ['invariant'], axis=1)
                _plot(inv_df, outDir, updates, nproc, inv)


def plot_06_model_comparison(df, outDir):

    def _plot(df, outDir, nproc):
        # Sorting
        df = df.sort_values(by=['tenants'])
        # Change units
        df['inv_time'] /= 1e6  # usec -> sec

        df = df.pivot(index='tenants', columns='model_only',
                      values='inv_time').reset_index()
        df = df.rename(columns={False: 'Neo', True: 'Model-based'})

        # Plot time comparison
        ax = df.plot(
            x='tenants',
            y=['Model-based', 'Neo'],
            kind='bar',
            legend=False,
            width=0.8,
            xlabel='',
            ylabel='',
            rot=0,
        )
        ax.legend(bbox_to_anchor=(1.0, 1.18),
                  ncol=2,
                  fontsize=20,
                  frameon=False,
                  fancybox=False)
        ax.grid(axis='y')
        ax.set_yscale('log')
        ax.set_xlabel('Tenants', fontsize=22)
        ax.set_ylabel('Time (seconds)', fontsize=22)
        ax.tick_params(axis='both', which='both', labelsize=22)
        ax.tick_params(axis='x', which='both', top=False, bottom=False)
        fig = ax.get_figure()
        fn = os.path.join(
            outDir,
            ('06.compare-model.inv-1.0-updates.' + str(nproc) + '-procs.pdf'))
        fig.savefig(fn, bbox_inches='tight')
        plt.close(fig)

    # Filter rows
    df = df[df.invariant == 1].drop(['invariant'], axis=1)
    df = df[df.drop_method == 'timeout'].drop(['drop_method'], axis=1)
    df = df[df.updates == 'None'].drop(['updates'], axis=1)
    # Filter columns
    df = df.drop([
        'inv_memory', 'num_nodes', 'num_links', 'num_updates', 'total_conn',
        'independent_cec', 'violated'
    ],
                 axis=1)

    for nproc in df.procs.unique():
        nproc_df = df[df.procs == nproc].drop(['procs'], axis=1)
        _plot(nproc_df, outDir, nproc)


def plot_06_stats(invDir, outDir, compare_model=False):
    df = pd.read_csv(os.path.join(invDir, 'stats.csv'))
    # Rename values
    df.loc[df['updates'] == 0, 'updates'] = 'None'
    df.loc[df['tenants'] == df['updates'] * 2, 'updates'] = 'Half-tenant'
    df.loc[df['tenants'] == df['updates'], 'updates'] = 'All-tenant'

    plot_06_perf_vs_tenants(df, outDir)
    plot_06_perf_vs_nprocs(df, outDir)
    plot_06_perf_vs_drop_methods(df, outDir)
    plot_06_model_comparison(df, outDir)


def plot_latency(df, outFn, sample_limit=None):
    df = df.reset_index()
    df.loc[:, 'latency'] = df['pkt_lat'] + df['drop_lat']

    # Filter columns
    df = df.drop([
        'overall_lat', 'rewind_lat', 'rewind_injections', 'pkt_lat',
        'drop_lat', 'tenants', 'updates', 'invariant', 'violated'
    ],
                 axis=1)

    df = df.pivot(columns='drop_method', values=['latency', 'timeout'])
    df.columns = ['_'.join(col) for col in df.columns.values]
    df = df.drop(['timeout_dropmon', 'timeout_ebpf'], axis=1)
    df = df.apply(lambda x: pd.Series(x.dropna().values))

    # Rename column titles
    df = df.rename(
        columns={
            'latency_dropmon': 'Latency (drop_mon)',
            'latency_ebpf': 'Latency (ebpf)',
            'latency_timeout': 'Latency (timeout)',
            'timeout_timeout': 'Drop timeout'
        })

    if sample_limit:
        df = df.sample(n=sample_limit).sort_index()

    ax = df.plot(
        y=[
            'Latency (drop_mon)', 'Latency (ebpf)', 'Latency (timeout)',
            'Drop timeout'
        ],
        kind='line',
        legend=False,
        xlabel='',
        ylabel='',
        rot=0,
        lw=2,
    )
    ax.legend(bbox_to_anchor=(1.1, 1.27),
              columnspacing=0.8,
              ncol=2,
              fontsize=18,
              frameon=False,
              fancybox=False)
    ax.grid(axis='y')
    # ax.set_yscale('log')
    ax.set_xlabel('Packet injections', fontsize=22)
    ax.set_ylabel('Latency (microseconds)', fontsize=22)
    ax.tick_params(axis='both', which='both', labelsize=22)
    ax.tick_params(axis='x',
                   which='both',
                   top=False,
                   bottom=False,
                   labelbottom=False)
    fig = ax.get_figure()
    fig.savefig(outFn, bbox_inches='tight')
    plt.close(fig)


def plot_06_latency(invDir, outDir):
    df = pd.read_csv(os.path.join(invDir, 'lat.csv'))
    # Rename values
    df.loc[df['updates'] == 0, 'updates'] = 'None'
    df.loc[df['tenants'] == df['updates'] * 2, 'updates'] = 'Half-tenant'
    df.loc[df['tenants'] == df['updates'], 'updates'] = 'All-tenant'
    # Filter rows
    df = df[df.procs == 1]
    # Filter columns
    df = df.drop([
        'num_nodes', 'num_links', 'num_updates', 'total_conn',
        'independent_cec', 'procs'
    ],
                 axis=1)

    # latency line charts
    plot_latency(df[df.pkt_lat > 0],
                 os.path.join(outDir, '06.latency.recv.pdf'),
                 sample_limit=300)
    plot_latency(df[df.drop_lat > 0],
                 os.path.join(outDir, '06.latency.drop.pdf'))

    # latency CDF
    # TODO: plot one with pkt_lat > 0 and one with drop_lat > 0 with varying drop methods?
    lat_df = df[df.updates == 'None'].drop(['updates'], axis=1)
    # lat_df = lat_df.loc[lat_df.rewind_injections != -1]
    print(lat_df.to_string())


def main():
    parser = argparse.ArgumentParser(description='Plotting for Neo')
    parser.add_argument('-t',
                        '--target',
                        help='Plot target',
                        type=str,
                        action='store',
                        required=True)
    parser.add_argument('-l',
                        '--latency',
                        help='Plot latency',
                        action='store_true',
                        default=False)
    args = parser.parse_args()

    logging.basicConfig(level=logging.INFO,
                        format='[%(levelname)s] %(message)s')

    targetDir = os.path.abspath(args.target)
    if not os.path.isdir(targetDir):
        raise Exception("'" + targetDir + "' is not a directory")
    sourceDir = os.path.abspath(os.path.dirname(__file__))
    outDir = os.path.join(sourceDir, 'figures')
    os.makedirs(outDir, exist_ok=True)

    if args.latency:
        plot_06_latency(targetDir, outDir)
    else:
        exp_id = os.path.basename(targetDir)[:2]
        if exp_id == '06':
            plot_06_stats(targetDir, outDir)
        else:
            raise Exception("Parser not implemented for experiment " + exp_id)


if __name__ == '__main__':
    main()
