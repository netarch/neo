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
colors = [
    '#1f77b4', '#ff7f0e', '#2ca02c', '#d62728', '#9467bd', '#8c564b',
    '#e377c2', '#7f7f7f', '#bcbd22', '#17becf'
]


def plot_02_perf_vs_apps_hosts(df, outDir):

    def _plot(df, outDir, nproc, drop, inv):
        # Filter columns
        df = df.drop(['violated'], axis=1)
        # Sorting
        df = df.sort_values(by=['apps', 'fault'])
        # Change units
        df['inv_time'] /= 1e6  # usec -> sec
        df['inv_memory'] /= 1024  # KiB -> MiB

        time_df = df.pivot(index='apps', columns='fault',
                           values='inv_time').reset_index()
        time_df = time_df.rename(columns={
            False: 'Time (passing)',
            True: 'Time (failing)',
        })
        mem_df = df.pivot(index='apps', columns='fault',
                          values='inv_memory').reset_index()
        mem_df = mem_df.rename(columns={
            False: 'Memory (passing)',
            True: 'Memory (failing)',
        })
        merged_df = pd.merge(time_df, mem_df, on=['apps'])

        # Plot time/memory
        ax = merged_df.plot(
            x='apps',
            y=['Time (passing)', 'Time (failing)'],
            kind='bar',
            legend=False,
            width=0.8,
            xlabel='',
            ylabel='',
            rot=0,
        )
        ax = merged_df.plot(
            x='apps',
            secondary_y=['Memory (passing)', 'Memory (failing)'],
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
                  bbox_to_anchor=(1.18, 1.3),
                  columnspacing=0.8,
                  ncol=2,
                  fontsize=20,
                  frameon=False,
                  fancybox=False)

        ax.grid(axis='y')
        # ax.set_yscale('log')
        ax.set_xlabel('Applications and hosts/app', fontsize=22)
        ax.set_ylabel('Time (seconds)', fontsize=22)
        ax.tick_params(axis='both', which='both', labelsize=22)
        ax.tick_params(axis='x', which='both', top=False, bottom=False)
        ax.right_ax.grid(axis='y')
        mem_start_val = (floor(df.inv_memory.min()) - 1) // 10 * 10
        ax.right_ax.set_ylim(bottom=mem_start_val)
        ax.right_ax.set_ylabel('Memory (MiB)', fontsize=22)
        ax.right_ax.tick_params(axis='both', which='both', labelsize=18)
        fig = ax.get_figure()
        fn = os.path.join(outDir, ('02.perf-apps.inv-' + str(inv) + '.' +
                                   str(nproc) + '-procs.' + drop + '.pdf'))
        fig.savefig(fn, bbox_inches='tight')
        plt.close('all')

    for nproc in df.procs.unique():
        nproc_df = df[df.procs == nproc].drop(['procs'], axis=1)

        for drop in nproc_df.drop_method.unique():
            d_df = nproc_df[nproc_df.drop_method == drop].drop(['drop_method'],
                                                               axis=1)

            for inv in d_df.invariant.unique():
                inv_df = d_df[d_df.invariant == inv].drop(['invariant'],
                                                          axis=1)
                _plot(inv_df, outDir, nproc, drop, inv)


def plot_02_perf_vs_nprocs(df, outDir):

    def _plot(df, outDir, drop, fault, inv):
        # Filter columns
        df = df.drop(['inv_memory', 'violated'], axis=1)
        # Sorting
        df = df.sort_values(by=['procs', 'apps'])
        # Change units
        df['inv_time'] /= 1e6  # usec -> sec
        # Rename values
        df['apps'] = df['apps'].astype(str) + ' apps/hosts'

        df = df.pivot(index='procs', columns='apps',
                      values='inv_time').reset_index()
        df.at[3, '4 apps/hosts'] = np.nan

        # Plot time vs nprocs
        ax = df.plot(
            x='procs',
            y=['4 apps/hosts', '8 apps/hosts', '12 apps/hosts'],
            kind='line',
            style=['^-', 'o-', 'D-', 's-', 'x-'],
            legend=False,
            xlabel='',
            ylabel='',
            rot=0,
            lw=LINE_WIDTH,
        )
        ax.legend(ncol=1, fontsize=20, frameon=False, fancybox=False)
        ax.grid(axis='y')
        # ax.set_yscale('log')
        ax.set_xlabel('Parallel processes', fontsize=22)
        ax.set_ylabel('Time (seconds)', fontsize=22)
        ax.tick_params(axis='both', which='both', labelsize=22)
        ax.tick_params(axis='x', which='both', top=False, bottom=False)
        fig = ax.get_figure()
        fn = os.path.join(
            outDir, ('02.perf-nproc.inv-' + str(inv) +
                     ('.failing.' if fault else '.passing.') + drop + '.pdf'))
        fig.savefig(fn, bbox_inches='tight')
        plt.close(fig)

    for drop in df.drop_method.unique():
        d_df = df[df.drop_method == drop].drop(['drop_method'], axis=1)

        for fault in d_df.fault.unique():
            f_df = d_df[d_df.fault == fault].drop(['fault'], axis=1)

            for inv in f_df.invariant.unique():
                inv_df = f_df[f_df.invariant == inv].drop(['invariant'],
                                                          axis=1)
                _plot(inv_df, outDir, drop, fault, inv)


def plot_02_perf_vs_drop_methods(df, outDir):

    def _plot(df, outDir, nproc, fault, inv):
        # Filter columns
        df = df.drop(['inv_memory', 'violated'], axis=1)
        # Sorting
        df = df.sort_values(by=['apps', 'drop_method'])
        # Change units
        df['inv_time'] /= 1e6  # usec -> sec
        # Rename values
        df = (df.replace('dropmon', 'drop_mon'))

        df = df.pivot(index='apps', columns='drop_method',
                      values='inv_time').reset_index()

        # Plot time w. drop methods
        ax = df.plot(
            x='apps',
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
        ax.set_xlabel('Applications and hosts/app', fontsize=22)
        ax.set_ylabel('Time (seconds)', fontsize=22)
        ax.tick_params(axis='both', which='both', labelsize=22)
        ax.tick_params(axis='x', which='both', top=False, bottom=False)
        fig = ax.get_figure()
        fn = os.path.join(outDir, ('02.perf-drop.inv-' + str(inv) +
                                   ('.failing.' if fault else '.passing.') +
                                   str(nproc) + '-procs.pdf'))
        fig.savefig(fn, bbox_inches='tight')
        plt.close(fig)

    for nproc in df.procs.unique():
        nproc_df = df[df.procs == nproc].drop(['procs'], axis=1)

        for fault in nproc_df.fault.unique():
            f_df = nproc_df[nproc_df.fault == fault].drop(['fault'], axis=1)

            for inv in f_df.invariant.unique():
                inv_df = f_df[f_df.invariant == inv].drop(['invariant'],
                                                          axis=1)
                _plot(inv_df, outDir, nproc, fault, inv)


def plot_02_stats(invDir, outDir):
    df = pd.read_csv(os.path.join(invDir, 'stats.csv'))
    # Filter columns
    df = df.drop([
        'hosts', 'num_nodes', 'num_links', 'num_updates', 'total_conn',
        'independent_cec'
    ],
                 axis=1)

    plot_02_perf_vs_apps_hosts(df, outDir)
    plot_02_perf_vs_nprocs(df, outDir)
    plot_02_perf_vs_drop_methods(df, outDir)


def plot_03_stats(invDir, outDir):
    df = pd.read_csv(os.path.join(invDir, 'stats.csv'))
    # Filter rows
    df = df[df.procs == 1]
    df = df[df.drop_method == 'timeout']
    df = df[df.invariant == 1]
    # Filter columns
    df = df.drop([
        'servers', 'procs', 'drop_method', 'num_nodes', 'num_links',
        'num_updates', 'total_conn', 'invariant', 'independent_cec', 'violated'
    ],
                 axis=1)
    # Sorting
    df = df.sort_values(by=['lbs', 'algorithm'])
    # Change units
    df['inv_time'] /= 1e6  # usec -> sec
    df['inv_memory'] /= 1024  # KiB -> MiB

    time_df = df.pivot(index='lbs', columns='algorithm',
                       values='inv_time').reset_index()
    time_df = time_df.rename(
        columns={
            'rr': 'Time (round-robin)',
            'dh': 'Time (dest-hashing)',
            'sh': 'Time (source-hashing)',
        })
    mem_df = df.pivot(index='lbs', columns='algorithm',
                      values='inv_memory').reset_index()
    mem_df = mem_df.rename(
        columns={
            'rr': 'Memory (round-robin)',
            'dh': 'Memory (dest-hashing)',
            'sh': 'Memory (source-hashing)',
        })
    merged_df = pd.merge(time_df, mem_df, on=['lbs'])

    # Plot time/memory
    ax = merged_df.plot(
        x='lbs',
        y=[
            'Time (round-robin)', 'Time (dest-hashing)',
            'Time (source-hashing)'
        ],
        kind='bar',
        legend=False,
        width=0.8,
        xlabel='',
        ylabel='',
        rot=0,
    )
    ax = merged_df.plot(
        x='lbs',
        secondary_y=[
            'Memory (round-robin)', 'Memory (dest-hashing)',
            'Memory (source-hashing)'
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
              bbox_to_anchor=(1.12, 1.28),
              columnspacing=0.8,
              ncol=2,
              fontsize=14,
              frameon=False,
              fancybox=False)

    ax.grid(axis='y')
    ax.set_yscale('log')
    ax.set_xlabel('LBs and servers/LB', fontsize=22)
    ax.set_ylabel('Time (seconds)', fontsize=22)
    ax.tick_params(axis='both', which='both', labelsize=22)
    ax.tick_params(axis='x', which='both', top=False, bottom=False)
    ax.right_ax.grid(axis='y')
    mem_start_val = (floor(df.inv_memory.min()) - 1) // 10 * 10
    ax.right_ax.set_ylim(bottom=mem_start_val)
    ax.right_ax.set_ylabel('Memory (MiB)', fontsize=22)
    ax.right_ax.tick_params(axis='both', which='both', labelsize=18)
    fig = ax.get_figure()
    fn = os.path.join(outDir, ('03.perf-lbs.pdf'))
    fig.savefig(fn, bbox_inches='tight')
    plt.close('all')


def plot_03_compare_unopt(invDir):
    df = pd.read_csv(os.path.join(invDir, 'stats.csv'))
    # Rename values
    df.loc[df['optimization'] == True, 'optimization'] = 'Opt.'
    df.loc[df['optimization'] == False, 'optimization'] = 'Unopt.'
    # Filter rows
    df = df[df.procs == 1]
    df = df[df.drop_method == 'timeout']
    df = df[df.invariant == 1]
    # Filter columns
    df = df.drop([
        'inv_memory', 'procs', 'drop_method', 'num_nodes', 'num_links',
        'num_updates', 'total_conn', 'invariant', 'independent_cec', 'violated'
    ],
                 axis=1)
    # Sorting
    df = df.sort_values(by=['lbs', 'algorithm'])
    # Change units
    df['inv_time'] /= 1e6  # usec -> sec

    df = df.pivot(index='lbs',
                  columns=['algorithm', 'optimization'],
                  values='inv_time').reset_index()
    with open(os.path.join(invDir, 'compare-unopt.txt'), 'w') as f:
        f.write(df.to_string())


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
        df.at[2, '4 tenants'] = np.nan
        df.at[3, '4 tenants'] = np.nan
        df.at[3, '8 tenants'] = np.nan

        # Plot time vs nprocs
        ax = df.plot(
            x='procs',
            y=[
                '4 tenants', '8 tenants', '16 tenants', '32 tenants',
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

        df = df.pivot(index='tenants', columns='model_only',
                      values='inv_time').reset_index()

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

    # Rename values
    df.loc[df['model_only'] == True, 'model_only'] = 'Model-based'
    df.loc[df['model_only'] == False, 'model_only'] = 'Neo'
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
    # Change units
    df['inv_time'] /= 1e6  # usec -> sec

    with open('compare-model.inv-1.no-updates.1-proc.txt', 'w') as f:
        df = df[df.procs == 1].drop(['procs'], axis=1)
        df = df.sort_values(by=['tenants'])
        df = df.pivot(index='tenants', columns='model_only',
                      values='inv_time').reset_index()
        f.write(df.to_string())

    # for nproc in df.procs.unique():
    #     nproc_df = df[df.procs == nproc].drop(['procs'], axis=1)
    #     _plot(nproc_df, outDir, nproc)


def plot_06_stats(invDir, outDir):
    df = pd.read_csv(os.path.join(invDir, 'stats.csv'))
    # Rename values
    df.loc[df['updates'] == 0, 'updates'] = 'None'
    df.loc[df['tenants'] == df['updates'] * 2, 'updates'] = 'Half-tenant'
    df.loc[df['tenants'] == df['updates'], 'updates'] = 'All-tenant'
    # Filter rows
    df = df[df.optimization == True].drop(['optimization'], axis=1)

    # plot_06_perf_vs_tenants(df, outDir)
    # plot_06_perf_vs_nprocs(df, outDir)
    # plot_06_perf_vs_drop_methods(df, outDir)
    plot_06_model_comparison(df, outDir)


def plot_latency(df, outFn, sample_limit=None):
    # Filter columns
    df = df.drop(['rewind_lat', 'pkt_lat', 'drop_lat'], axis=1)

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


def plot_latency_cdf(df,
                     outDir,
                     exp_id,
                     logscale_for_reset=False,
                     logscale_for_no_reset=False):
    df.loc[:, 'latency_with_reset'] = df['latency'] + df['rewind_lat']

    # Filter columns
    df = df.drop(['rewind_lat', 'pkt_lat', 'drop_lat', 'timeout'], axis=1)

    df = df.pivot(columns='drop_method',
                  values=['latency', 'latency_with_reset'])
    df.columns = ['_'.join(col) for col in df.columns.values]
    df = df.apply(lambda x: pd.Series(x.dropna().values))

    no_reset_df = df.drop([
        'latency_with_reset_dropmon', 'latency_with_reset_ebpf',
        'latency_with_reset_timeout'
    ],
                          axis=1).rename(
                              columns={
                                  'latency_dropmon': 'dropmon',
                                  'latency_ebpf': 'ebpf',
                                  'latency_timeout': 'timeout',
                              })
    reset_df = df.drop(['latency_dropmon', 'latency_ebpf', 'latency_timeout'],
                       axis=1).rename(
                           columns={
                               'latency_with_reset_dropmon': 'dropmon',
                               'latency_with_reset_ebpf': 'ebpf',
                               'latency_with_reset_timeout': 'timeout',
                           })

    def _get_cdf_df(df, col):
        df = df[col].to_frame().dropna()
        df = df.sort_values(by=[col]).reset_index().rename(
            columns={col: 'latency'})
        df[col] = df.index + 1
        df = df[df.columns.drop(list(df.filter(regex='index')))]
        return df

    for (df, with_reset) in [(no_reset_df, False), (reset_df, True)]:
        # For each drop_method x with/without reset, get the latency CDF
        cdf_df = pd.DataFrame()
        peak_latencies = []

        for col in list(df.columns):
            df1 = _get_cdf_df(df, col)
            peak_latencies.append(df1.latency.iloc[-1])
            if cdf_df.empty:
                cdf_df = df1
            else:
                cdf_df = pd.merge(cdf_df, df1, how='outer', on='latency')
            del df1

        df = cdf_df.sort_values(by='latency').interpolate(
            limit_area='inside').rename(columns={
                'dropmon': 'drop_mon',
                'ebpf': 'ebpf',
                'timeout': 'timeout',
            })
        del cdf_df

        ax = df.plot(
            x='latency',
            y=['drop_mon', 'ebpf', 'timeout'],
            kind='line',
            legend=False,
            xlabel='',
            ylabel='',
            rot=0,
            lw=LINE_WIDTH,
        )
        ax.legend(bbox_to_anchor=(1.07, 1.2),
                  columnspacing=0.8,
                  ncol=3,
                  fontsize=22,
                  frameon=False,
                  fancybox=False)
        ax.grid(axis='both')
        if ((with_reset and logscale_for_reset)
                or (not with_reset and logscale_for_no_reset)):
            ax.set_xscale('log')
        ax.set_xlabel('Latency (microseconds)', fontsize=24)
        ax.set_ylabel('Packet injections', fontsize=24)
        ax.tick_params(axis='both', which='both', labelsize=24)
        ax.tick_params(axis='x', which='both', top=False, bottom=False)
        i = 0
        for peak_lat in peak_latencies:
            ax.axvline(peak_lat,
                       ymax=0.95,
                       linestyle='--',
                       linewidth=LINE_WIDTH - 1,
                       color=colors[i])
            i += 1
        fig = ax.get_figure()
        fig.savefig(os.path.join(
            outDir, (exp_id + '.latency-cdf' +
                     ('.with-reset' if with_reset else '') + '.pdf')),
                    bbox_inches='tight')
        plt.close(fig)


def plot_02_latency(invDir, outDir):
    df = pd.read_csv(os.path.join(invDir, 'lat.csv'))
    # Merge latency values
    df.loc[:, 'latency'] = df['pkt_lat'] + df['drop_lat']
    # Filter rows
    df = df[df.procs == 1]
    # Filter columns
    df = df.drop([
        'overall_lat', 'rewind_injections', 'hosts', 'procs', 'fault',
        'num_nodes', 'num_links', 'num_updates', 'total_conn', 'invariant',
        'independent_cec', 'violated'
    ],
                 axis=1)
    df = df.reset_index().drop(['index'], axis=1)

    # latency line charts
    recv_df = df[df.pkt_lat > 0]
    for apps in recv_df.apps.unique():
        plot_latency(
            recv_df[recv_df.apps == apps],
            os.path.join(outDir,
                         ('02.latency.' + str(apps) + '-apps.recv.pdf')),
            sample_limit=None)
    plot_latency(df[df.drop_lat > 0],
                 os.path.join(outDir, '02.latency.drop.pdf'))

    # latency CDF
    plot_latency_cdf(df, outDir, os.path.basename(invDir)[:2])


def plot_03_latency(invDir, outDir):
    df = pd.read_csv(os.path.join(invDir, 'lat.csv'))
    # Merge latency values
    df.loc[:, 'latency'] = df['pkt_lat'] + df['drop_lat']
    # Filter rows
    df = df[df.procs == 1]
    df = df[df.drop_method == 'timeout']
    df = df[df.invariant == 1]
    # Filter columns
    df = df.drop([
        'overall_lat', 'servers', 'algorithm', 'procs', 'drop_method',
        'num_nodes', 'num_links', 'num_updates', 'total_conn', 'invariant',
        'independent_cec', 'violated'
    ],
                 axis=1)
    df = df.reset_index().drop(['index'], axis=1)

    def _plot_latency(df, outFn):
        # Filter columns
        df = df.drop(
            ['rewind_lat', 'rewind_injections', 'pkt_lat', 'drop_lat', 'lbs'],
            axis=1)
        # Rename column titles
        df = df.rename(columns={
            'latency': 'Latency',
            'timeout': 'Drop timeout'
        })

        ax = df.plot(
            y=['Latency', 'Drop timeout'],
            kind='line',
            legend=False,
            xlabel='',
            ylabel='',
            rot=0,
            lw=2,
        )
        ax.legend(bbox_to_anchor=(1.0, 1.2),
                  columnspacing=0.8,
                  ncol=2,
                  fontsize=22,
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

    def _plot_latency_cdf(df, outFn):
        df.loc[:, 'latency_w_reset'] = df['latency'] + df['rewind_lat']
        # Filter columns
        df = df.drop([
            'rewind_lat', 'rewind_injections', 'pkt_lat', 'drop_lat',
            'timeout', 'lbs'
        ],
                     axis=1)
        # Rename column titles
        df = df.rename(columns={
            'latency': 'Exclude reset',
            'latency_w_reset': 'Include reset'
        })

        def __get_cdf_df(df, col):
            df = df[col].to_frame().dropna()
            df = df.sort_values(by=[col]).reset_index().rename(
                columns={col: 'latency'})
            df[col] = df.index + 1
            df = df[df.columns.drop(list(df.filter(regex='index')))]
            return df

        cdf_df = pd.DataFrame()
        peak_latencies = []

        for col in list(df.columns):
            df1 = __get_cdf_df(df, col)
            peak_latencies.append(df1.latency.iloc[-1])
            if cdf_df.empty:
                cdf_df = df1
            else:
                cdf_df = pd.merge(cdf_df, df1, how='outer', on='latency')
            del df1

        df = cdf_df.sort_values(by='latency').interpolate(limit_area='inside')
        del cdf_df

        # # Change units
        # df['latency'] /= 1e6  # usec -> sec
        # peak_latencies = [l / 1e6 for l in peak_latencies]

        ax = df.plot(
            x='latency',
            y=['Exclude reset', 'Include reset'],
            kind='line',
            legend=False,
            xlabel='',
            ylabel='',
            rot=0,
            lw=LINE_WIDTH,
        )
        ax.legend(bbox_to_anchor=(1.07, 1.2),
                  columnspacing=0.8,
                  ncol=2,
                  fontsize=22,
                  frameon=False,
                  fancybox=False)
        ax.grid(axis='both')
        ax.set_xscale('log')
        ax.set_xlabel('Latency (seconds)', fontsize=24)
        ax.set_ylabel('Packet injections', fontsize=24)
        ax.tick_params(axis='both', which='both', labelsize=24)
        ax.tick_params(axis='x', which='both', top=False, bottom=False)
        i = 0
        for peak_lat in peak_latencies:
            ax.axvline(peak_lat,
                       ymax=0.95,
                       linestyle='--',
                       linewidth=LINE_WIDTH - 1,
                       color=colors[i])
            i += 1
        fig = ax.get_figure()
        fig.savefig(outFn, bbox_inches='tight')
        plt.close(fig)

    tbl = {}

    for lbs in df.lbs.unique():
        lbs_df = df[df.lbs == lbs]
        injections = lbs_df.rewind_injections
        tbl[lbs] = {'lbs': lbs}
        tbl[lbs]['num_reset'] = injections[injections != -1].count()
        tbl[lbs]['num_reset_injections'] = injections[injections != -1].sum()

        # latency line charts
        _plot_latency(
            lbs_df,
            os.path.join(outDir, ('03.latency.' + str(lbs) + '-lbs.pdf')))

    # latency CDF
    _plot_latency_cdf(df, os.path.join(outDir, '03.latency-cdf.pdf'))

    print(pd.DataFrame(tbl))


def plot_06_latency(invDir, outDir):
    df = pd.read_csv(os.path.join(invDir, 'lat.csv'))
    # Merge latency values
    df.loc[:, 'latency'] = df['pkt_lat'] + df['drop_lat']
    # Rename values
    df.loc[df['updates'] == 0, 'updates'] = 'None'
    df.loc[df['tenants'] == df['updates'] * 2, 'updates'] = 'Half-tenant'
    df.loc[df['tenants'] == df['updates'], 'updates'] = 'All-tenant'
    # Filter rows
    df = df[df.procs == 1]
    df = df[df.optimization == True]
    # Filter columns
    df = df.drop([
        'overall_lat', 'rewind_injections', 'tenants', 'updates', 'procs',
        'num_nodes', 'num_links', 'num_updates', 'total_conn', 'invariant',
        'independent_cec', 'violated', 'optimization'
    ],
                 axis=1)
    df = df.reset_index().drop(['index'], axis=1)

    # latency line charts
    plot_latency(df[df.pkt_lat > 0],
                 os.path.join(outDir, '06.latency.recv.pdf'),
                 sample_limit=300)
    plot_latency(df[df.drop_lat > 0],
                 os.path.join(outDir, '06.latency.drop.pdf'))

    # latency CDF
    plot_latency_cdf(df,
                     outDir,
                     os.path.basename(invDir)[:2],
                     logscale_for_reset=True)


def plot_06_compare_unopt(invDir, outDir):

    def _plot(df, outDir, drop, inv):
        # Filter columns
        df = df.drop([
            'inv_memory', 'procs', 'num_nodes', 'num_links', 'num_updates',
            'total_conn', 'independent_cec', 'violated', 'model_only'
        ],
                     axis=1)
        # Sorting
        df = df.sort_values(by=['tenants', 'updates'])
        # Change units
        df['inv_time'] /= 1e6  # usec -> sec
        df['opt_update'] = df.optimization + ' (' + df.updates + ')'

        df = df.pivot(index='tenants', columns='opt_update',
                      values='inv_time').reset_index()

        ax = df.plot(
            x='tenants',
            y=[
                'Opt. (all-tenant)',
                'Unopt. (all-tenant)',
                'Opt. (half-tenant)',
                'Unopt. (half-tenant)',
                'Opt. (none)',
                'Unopt. (none)',
            ],
            kind='bar',
            legend=False,
            width=0.8,
            xlabel='',
            ylabel='',
            rot=0,
        )
        ax.legend(bbox_to_anchor=(1.14, 1.2),
                  columnspacing=0.8,
                  ncol=3,
                  fontsize=13,
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
            outDir, ('06.compare-unopt.inv-' + str(inv) + '.' + drop + '.pdf'))
        fig.savefig(fn, bbox_inches='tight')
        plt.close('all')

    df = pd.read_csv(os.path.join(invDir, 'stats.csv'))
    # Rename values
    df.loc[df['updates'] == 0, 'updates'] = 'none'
    df.loc[df['tenants'] == df['updates'] * 2, 'updates'] = 'half-tenant'
    df.loc[df['tenants'] == df['updates'], 'updates'] = 'all-tenant'
    df.loc[df['optimization'] == True, 'optimization'] = 'Opt.'
    df.loc[df['optimization'] == False, 'optimization'] = 'Unopt.'
    # Filter rows
    df = df[df.model_only != True]
    df = df[df.procs == 1]

    for drop in df.drop_method.unique():
        d_df = df[df.drop_method == drop].drop(['drop_method'], axis=1)

        for inv in d_df.invariant.unique():
            inv_df = d_df[d_df.invariant == inv].drop(['invariant'], axis=1)
            _plot(inv_df, outDir, drop, inv)


def main():
    parser = argparse.ArgumentParser(description='Plotting for Neo')
    parser.add_argument('-t',
                        '--target',
                        help='Plot target',
                        type=str,
                        action='store',
                        required=True)
    args = parser.parse_args()

    logging.basicConfig(level=logging.INFO,
                        format='[%(levelname)s] %(message)s')

    targetDir = os.path.abspath(args.target)
    if not os.path.isdir(targetDir):
        raise Exception("'" + targetDir + "' is not a directory")
    sourceDir = os.path.abspath(os.path.dirname(__file__))
    outDir = os.path.join(sourceDir, 'figures')
    os.makedirs(outDir, exist_ok=True)

    exp_id = os.path.basename(targetDir)[:2]
    if exp_id == '02':
        plot_02_stats(targetDir, outDir)
        plot_02_latency(targetDir, outDir)
    elif exp_id == '03':
        plot_03_stats(targetDir, outDir)
        plot_03_latency(targetDir, outDir)
        plot_03_compare_unopt(targetDir)
    elif exp_id == '06':
        plot_06_stats(targetDir, outDir)
        # plot_06_latency(targetDir, outDir)
        # plot_06_compare_unopt(targetDir, outDir)
    else:
        raise Exception("Parser not implemented for experiment " + exp_id)


if __name__ == '__main__':
    main()
