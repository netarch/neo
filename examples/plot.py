#!/usr/bin/env python3

import argparse
import os
import re
import logging
import numpy as np
import pandas as pd
from matplotlib import pyplot as plt
from math import floor

LINE_WIDTH = 7
colors = [
    '#1f77b4', '#ff7f0e', '#2ca02c', '#d62728', '#9467bd', '#8c564b',
    '#e377c2', '#7f7f7f', '#bcbd22', '#17becf'
]


def values_compare(series):
    ids = {
        # methods
        'Mono.': 0,
        'Libra': 1,
        'VF-like': 2,
        'Scylla (D)': 3,
        'Scylla': 4,
        # networks
        'N5': 5,
        'N4': 6,
        'N3': 7,
        'N2': 8,
        'N1': 9,
        'N5 (underlay)': 10,
        'N4 (underlay)': 11,
        'N3 (underlay)': 12,
        'N2 (underlay)': 13,
        'N1 (underlay)': 14,
    }
    return series.apply(lambda m: ids[m])


def rewrite_values(df):
    return (df
        # networks
        .replace('nsxt',                'N1')
        .replace('wwtatc',              'N2')
        .replace('field-demo',          'N3')
        .replace('scale',               'N4')
        .replace('vmware-it',           'N5')
        .replace('nsxt-underlay',       'N1 (underlay)')
        .replace('wwtatc-underlay',     'N2 (underlay)')
        .replace('field-demo-underlay', 'N3 (underlay)')
        .replace('scale-underlay',      'N4 (underlay)')
        .replace('vmware-it-underlay',  'N5 (underlay)')
        # methods
        .replace('monolithic',                  'Mono.')
        .replace('libra',                       'Libra')
        .replace('veriflow',                    'VF-like')
        .replace('incremental',                 'Scylla')
        .replace('incremental-device-level',    'Scylla (D)')
    )


def reformat_string_for_filenames(s):
    s = s.lower() \
        .replace('-like', '') \
        .replace(' (', '-') \
        .replace(')', '')
    s = re.sub(r'n([0-9]+)', r'N\1', s)
    return s


def plot_single_node_comparison(intentType, new_intent, df, outDir):
    # Filter rows
    df = df[df.method != 'Libra']
    df = df.loc[-df.method.str.contains('\(D\)')]
    df = df.loc[-df.network.str.contains('underlay')]
    # Filter columns
    df = df.drop(['slice', 'total_slices', 'timestamp'], axis=1)
    df['name'] = df.method + ' / ' + df.network
    # Sorting
    df = df.sort_values(by=['method', 'network'],
                        key=values_compare,
                        ascending=False)
    # Rename column titles
    df = df.rename(
        columns={
            'total_time': 'Total time',
            'intent_check_time': 'Intent check time',
            'model_build_time': 'Model build time',
            'disk_IO_time': 'Disk I/O',
            'memory': 'Memory'
        })

    ##
    ## Time
    ##
    for network in df.network.unique():
        net_df = df[df.network == network]
        ax = net_df.plot(
            x='method',
            y=[
                'Disk I/O', 'Model build time', 'Intent check time',
                'Total time'
            ],
            kind='bar',
            figsize=(10, 8.2),
            legend=False,
            width=0.8,
            xlabel='',
            ylabel='',
            rot=0,
        )
        # hatches = itertools.cycle(['', '//', '.', '/', 'x', 'o', 'O', '+', '-', '*'])
        # for i, bar in enumerate(ax.patches):
        #     if i % len(net_df.method.unique()) == 0:
        #         hatch = next(hatches)
        #     bar.set_hatch(hatch)
        ax.grid(axis='y')
        ax.legend(bbox_to_anchor=(0.41, 1.4),
                  ncol=2,
                  fontsize=34,
                  columnspacing=0.5)
        ax.set_yscale('log')
        ax.set_ylabel('Time (seconds)', fontsize=45)
        ax.tick_params(axis='both', which='both', labelsize=40)
        ax.tick_params(axis='x', which='both', top=False, bottom=False)
        fig = ax.get_figure()
        fn = os.path.join(
            outDir,
            ('single-node.time.' + intentType +
             ('.first-run.' if new_intent else '.update.') + network + '.pdf'))
        fig.savefig(fn)

    ##
    ## Memory
    ##
    df_mem = df.pivot(index='network', columns='method',
                      values='Memory').reset_index()
    ax = df_mem.plot(
        x='network',
        y=df.method.unique(),
        kind='bar',
        legend=False,
        width=0.8,
        xlabel='',
        ylabel='',
        rot=0,
    )
    ax.grid(axis='y')
    ax.legend(bbox_to_anchor=(0.4, 1.28),
              ncol=len(df.method.unique()),
              fontsize=40,
              columnspacing=0.5)
    ax.set_yscale('log')
    ax.set_ylabel('Memory (GiB)', fontsize=50)
    ax.tick_params(axis='both', which='both', labelsize=45)
    ax.tick_params(axis='x',
                   which='both',
                   top=False,
                   bottom=False,
                   labelsize=45)
    fig = ax.get_figure()
    fn = os.path.join(outDir,
                      ('single-node.memory.' + intentType +
                       ('.first-run' if new_intent else '.update') + '.pdf'))
    fig.savefig(fn)


def plot_Libra_comparison(intentType, new_intent, df, outDir):
    # Filter rows
    df = df.loc[df.network.str.contains('underlay')]
    df = df.loc[-df.method.str.contains('\(D\)')]
    df = df.loc[-df.method.str.contains('VeriFlow')]
    # Some intents are not tested with underlay networks. Ignore them
    if df.empty or intentType != 'reachability':
        return
    # Rename values
    df = df.replace(to_replace=' \(underlay\)', value='', regex=True)
    # Aggregate across shards with max(time) and sum(memory)
    grouped = df.groupby(by=['network', 'method', 'total_slices'],
                         as_index=False)
    df = df.merge(grouped['total_time'].agg(np.max), how='inner', sort=False)
    df = df.merge(grouped['memory'].agg(np.sum),
                  how='inner',
                  on=['network', 'method', 'total_slices'],
                  sort=False)
    df = df.merge(grouped['memory'].agg(np.mean),
                  how='inner',
                  on=['network', 'method', 'total_slices'],
                  sort=False)
    df = df.drop(['memory_x'], axis=1)
    df = df.rename(columns={
        'memory_y': 'Overall',
        'memory': 'Each shard',
    })
    # Sorting
    df = df.sort_values(by=['network', 'method'],
                        key=values_compare,
                        ascending=False)
    # Update values
    df.loc[df.method == 'Libra',
           'method'] = (df.method + ' (' + df.total_slices.apply(str) + ')')
    # Filter rows
    df = df.loc[df.total_slices != 2]
    df = df.loc[df.total_slices != 4]
    # Filter columns
    df = df.drop(['slice', 'total_slices'], axis=1)
    # Rename column titles
    df = df.rename(
        columns={
            'total_time': 'Total time',
            'intent_check_time': 'Intent check time',
            'model_build_time': 'Model build time',
            'disk_IO_time': 'Disk I/O',
        })

    ##
    ## Time
    ##
    for network in df.network.unique():
        net_df = df[df.network == network]
        ax = net_df.plot(
            x='method',
            y=[
                'Disk I/O', 'Model build time', 'Intent check time',
                'Total time'
            ],
            kind='bar',
            figsize=(12, 7.5),
            legend=False,
            width=0.7,
            xlabel='',
            ylabel='Time (seconds)',
            rot=0,
        )
        ax.grid(axis='y')
        ax.legend(bbox_to_anchor=(0.5, 1.12), ncol=4, fontsize='x-small')
        ax.set_yscale('log')
        ax.set_ylabel('Time (seconds)', fontsize='small')
        ax.tick_params(axis='both', which='both', labelsize='x-small')
        ax.tick_params(axis='x', which='both', top=False, bottom=False)
        fig = ax.get_figure()
        fn = os.path.join(
            outDir,
            ('libra.time.' + intentType +
             ('.first-run.' if new_intent else '.update.') + network + '.pdf'))
        fig.savefig(fn)
        plt.close(fig)

    ##
    ## Memory
    ##
    df = df.pivot(index='network', columns='method',
                  values='Each shard').reset_index()
    df = df.rename(columns={'Libra (8)': 'Libra (per shard)'})
    ax = df.plot(
        x='network',
        y=['Scylla', 'Libra (per shard)', 'Monolithic'],
        kind='bar',
        legend=False,
        width=0.76,
        xlabel='',
        ylabel='',
        rot=0,
    )
    ax.grid(axis='y')
    ax.legend(bbox_to_anchor=(0.5, 1.12), ncol=4, fontsize='xx-small')
    ax.set_yscale('log')
    ax.set_ylabel('Memory (GiB)', fontsize='small')
    ax.tick_params(axis='both', which='both', labelsize='small')
    ax.tick_params(axis='x', which='both', left=False, right=False)
    fig = ax.get_figure()
    fn = os.path.join(outDir,
                      ('libra.memory.' + intentType +
                       ('.first-run.' if new_intent else '.update.') + 'pdf'))
    fig.savefig(fn)
    plt.close(fig)


def plot_granularity_comparison(intentType, new_intent, df, outDir):
    # Filter rows
    df = df.loc[-df.network.str.contains('underlay')]
    df = df.loc[df.method.str.contains('Scylla')]
    # Filter columns
    df = df.drop(['slice', 'total_slices', 'timestamp'], axis=1)
    # Sorting
    df = df.sort_values(by=['network', 'method'],
                        key=values_compare,
                        ascending=False)
    # Rename values
    df = (df.replace('Scylla (D)', 'Device-lvl'))
    df = (df.replace('Scylla', 'Rule-lvl'))
    df['name'] = df.method + '\n' + df.network
    # Rename column titles
    df = df.rename(
        columns={
            'total_time': 'Total time',
            'intent_check_time': 'Intent check time',
            'model_build_time': 'Model build time',
            'disk_IO_time': 'Disk I/O',
            'memory': 'Memory'
        })

    ##
    ## Time but only N2 and N5
    ##
    small_df = df.loc[-(df.network != 'N2') | -(df.network != 'N5')]
    ax = small_df.plot(
        x='name',
        y=['Disk I/O', 'Model build time', 'Intent check time', 'Total time'],
        kind='bar',
        legend=False,
        width=0.8,
        figsize=(10, 8),
        xlabel='',
        ylabel='',
        rot=0,
    )
    ax.grid(axis='x')
    ax.legend(bbox_to_anchor=(0.45, 1.35), ncol=2, fontsize=28)
    ax.set_yscale('log')
    ax.set_ylabel('Time (seconds)', fontsize=40)
    ax.tick_params(axis='both', which='both', labelsize=28)
    ax.tick_params(axis='x', which='both', left=False, right=False)
    fig = ax.get_figure()
    fn = os.path.join(outDir,
                      ('granularity.N2N5.time.' + intentType +
                       ('.first-run' if new_intent else '.update') + '.pdf'))
    fig.savefig(fn)

    ##
    ## Time
    ##
    df['name'] = df.method + ' - ' + df.network
    ax = df.plot(
        x='name',
        y=['Disk I/O', 'Model build time', 'Intent check time', 'Total time'],
        kind='barh',
        legend=False,
        width=0.85,
        figsize=(10, 9),
        xlabel='',
        ylabel='',
        rot=0,
    )
    ax.grid(axis='x')
    ax.legend(bbox_to_anchor=(0.3, 1.25), ncol=2, fontsize=25)
    ax.set_xscale('log')
    ax.set_xlabel('Time (seconds)', fontsize=30)
    ax.tick_params(axis='both', which='both', labelsize=25)
    ax.tick_params(axis='y', which='both', left=False, right=False)
    fig = ax.get_figure()
    fn = os.path.join(outDir,
                      ('granularity.time.' + intentType +
                       ('.first-run' if new_intent else '.update') + '.pdf'))
    fig.savefig(fn)

    ##
    ## Memory
    ##
    df_mem = df.pivot(index='network', columns='method',
                      values='Memory').reset_index()
    ax = df_mem.plot(
        x='network',
        y=df.method.unique(),
        kind='bar',
        figsize=(11, 8),
        legend=False,
        width=0.8,
        xlabel='',
        ylabel='',
        rot=0,
    )
    ax.grid(axis='y')
    ax.legend(bbox_to_anchor=(0.5, 1.25),
              ncol=len(df.method.unique()),
              fontsize=40,
              columnspacing=0.5)
    # ax.set_yscale('log')
    ax.set_ylabel('Memory (GiB)', fontsize=50)
    ax.tick_params(axis='both', which='both', labelsize=45)
    ax.tick_params(axis='x', which='both', top=False, bottom=False)
    fig = ax.get_figure()
    fn = os.path.join(outDir,
                      ('granularity.memory.' + intentType +
                       ('.first-run' if new_intent else '.update') + '.pdf'))
    fig.savefig(fn)


def plot_local_single_intent(baseDir, outDir):
    df = pd.read_csv(os.path.join(baseDir, 'results.single-intent.csv'))
    df = rewrite_values(df)

    for intentType in df.intent_type.unique():
        int_df = df[df.intent_type == intentType].drop(['intent_type'], axis=1)

        for new_intent in int_df.new_intent.unique():
            ni_df = int_df[int_df.new_intent == new_intent].drop(
                ['new_intent'], axis=1)
            plot_single_node_comparison(intentType, new_intent, ni_df, outDir)
            plot_Libra_comparison(intentType, new_intent, ni_df, outDir)
            plot_granularity_comparison(intentType, new_intent, ni_df, outDir)


def plot_multi_intent_time(df, outDir, network):
    # Filter rows
    df = df.loc[-df.method.str.contains('\(D\)')]
    # Filter columns
    df = df.drop([
        'network', 'slice', 'intent_type', 'timestamp', 'intent',
        'intent_check_time', 'model_build_time', 'disk_IO_time', 'memory'
    ],
                 axis=1)
    # Sorting
    df = df.sort_values(by=['method'], key=values_compare, ascending=False)
    network = reformat_string_for_filenames(network)

    def get_time_cdf_df(df, method, extrapolate_factor=1):
        # Filter columns
        df = df['total_time'].to_frame()
        # Get number of updates/intents
        df = df.sort_values(by=['total_time']).reset_index().rename(
            columns={'total_time': 'time'})
        df[method] = df.index + 1
        if extrapolate_factor > 1:
            df.update(df[method].apply(lambda x: x * extrapolate_factor))
        df = df[df.columns.drop(list(df.filter(regex='index')))]
        return df

    for new_intent in df.new_intent.unique():
        time_df = df[df.new_intent == new_intent]
        time_df = time_df.drop(['new_intent'], axis=1)

        # For each method, get the time vs num_intents/updates DF
        cdf_df = pd.DataFrame()
        peak_times = []

        for method in time_df.method.unique():
            method_df = time_df[time_df.method == method]
            method_df = method_df.drop(['method'], axis=1)
            df1 = pd.DataFrame()
            if 'Scylla' in method:
                if 'underlay' in network:  # underlay network, comparison with Libra with 8 shards
                    df1 = get_time_cdf_df(method_df, method + ' (8)', 8)
                else:
                    df1 = get_time_cdf_df(method_df, method)
            elif method == 'Libra':
                method_df = method_df[method_df.total_slices == 8]
                df1 = get_time_cdf_df(method_df, method + ' (8)')
            elif 'underlay' not in network:
                df1 = get_time_cdf_df(method_df, method)
            if not df1.empty:
                peak_times.append(df1.time.iloc[-1])
                if cdf_df.empty:
                    cdf_df = df1
                else:
                    cdf_df = pd.merge(cdf_df, df1, how='outer', on='time')
            del df1
            del method_df

        time_df = cdf_df.sort_values(by='time').interpolate(
            limit_area='inside')
        del cdf_df
        columns = list(time_df.columns.drop(['time']))
        ax = time_df.plot(
            x='time',
            y=columns,
            kind='line',
            figsize=(10, 9),
            legend=False,
            xlabel='',
            ylabel='',
            rot=0,
            lw=LINE_WIDTH,
        )
        ax.grid(axis='both')
        ax.legend(bbox_to_anchor=(0.44, 1.25),
                  ncol=len(columns),
                  fontsize=40,
                  columnspacing=0.7)
        ax.set_xscale('log')
        ax.set_xlabel('Time (seconds)', fontsize=50)
        ax.set_ylabel('Number of intents', fontsize=50)
        ax.tick_params(axis='both', which='both', labelsize=40)
        # ax.tick_params(axis='x', which='both', labelsize=40, labelrotation=40)
        ax.tick_params(axis='y', which='minor', left=False, right=False)
        i = 0
        for peak_time in peak_times:
            ax.axvline(peak_time,
                       ymax=0.95,
                       linestyle='--',
                       linewidth=LINE_WIDTH - 2,
                       color=colors[i])
            i += 1
        fig = ax.get_figure()
        fn = os.path.join(
            outDir,
            ('time.' +
             ('first-run.' if new_intent else 'update.') + network + '.pdf'))
        fig.savefig(fn)
        plt.close(fig)


def plot_multi_intent_memory(df, outDir, network):
    # Filter rows
    df = df.loc[-df.method.str.contains('\(D\)')]
    df = df[df.new_intent == True]
    # Filter columns
    df = df.drop([
        'network', 'slice', 'intent_type', 'new_intent', 'total_time',
        'intent_check_time', 'model_build_time', 'disk_IO_time'
    ],
                 axis=1)
    # Aggregate across shards with sum(memory)
    grouped = df.groupby(by=['method', 'total_slices', 'intent'],
                         as_index=False)
    df = grouped['memory'].agg(np.sum)
    # Sorting
    df = df.sort_values(by=['method'], key=values_compare, ascending=False)

    def get_memory_df(df, method, extrapolate_factor=1):
        assert len(df.total_slices.unique()) == 1
        total_slices = df.total_slices.unique()[0]
        # Filter columns
        df = df.drop(['total_slices', 'intent'], axis=1)
        df = df.rename(columns={'memory': method})
        # Sorting
        df = df.sort_values(by=[method])
        # Reset index for count
        df = df.reset_index().drop(['index'], axis=1)
        df['num_intents'] = df.index + 1
        if 'Libra' in method:
            df.num_intents *= total_slices
        # Extrapolate
        if extrapolate_factor > 1:
            df.num_intents *= extrapolate_factor
            df[method] *= extrapolate_factor
        return df

    # For each method, get the memory vs num_updates DF
    mem_df = pd.DataFrame()

    for method in df.method.unique():
        method_df = df[df.method == method]
        method_df = method_df.drop(['method'], axis=1)
        df1 = pd.DataFrame()
        if 'Scylla' in method:
            if 'underlay' in network:
                df1 = get_memory_df(method_df, method + ' (8)', 8)
            else:
                df1 = get_memory_df(method_df, method)
        elif method == 'Libra':
            method_df = method_df[method_df.total_slices == 8]
            df1 = get_memory_df(method_df, method + ' (8)')
        elif 'underlay' not in network:
            df1 = get_memory_df(method_df, method)
        if not df1.empty:
            if mem_df.empty:
                mem_df = df1
            else:
                mem_df = pd.merge(mem_df, df1, how='outer', on='num_intents')
        del df1
        del method_df

    df = mem_df
    del mem_df

    network = reformat_string_for_filenames(network)
    columns = list(df.columns.drop(['num_intents']))
    ax = df.plot(
        x='num_intents',
        y=columns,
        kind='line',
        figsize=(10, 9),
        legend=False,
        xlabel='',
        ylabel='',
        rot=0,
        lw=LINE_WIDTH,
    )
    ax.grid(axis='both')
    ax.legend(bbox_to_anchor=(0.44, 1.25),
              ncol=len(columns),
              fontsize=40,
              columnspacing=0.7)
    ax.set_xlabel('Number of intents', fontsize=50)
    ax.set_ylabel('Memory (GiB)', fontsize=50)
    ax.tick_params(axis='both', which='both', labelsize=40)
    ax.tick_params(axis='x', which='minor', left=False, right=False)
    fig = ax.get_figure()
    fn = os.path.join(outDir, ('memory.first-run.' + network + '.pdf'))
    fig.savefig(fn)
    plt.close(fig)


def plot_multi_intent_granularity(df, outDir, network):
    # Filter rows
    df = df.loc[-df.network.str.contains('underlay')]
    df = df.loc[df.method.str.contains('Scylla')]
    # Filter columns
    df = df.drop([
        'network', 'slice', 'total_slices', 'intent_type', 'intent_check_time',
        'model_build_time', 'disk_IO_time'
    ],
                 axis=1)
    # Sorting
    df = df.sort_values(by=['method'], key=values_compare, ascending=False)

    if df.empty:
        return

    #
    # Time
    #
    def get_time_cdf_df(df, method):
        # Filter columns
        df = df['total_time'].to_frame()
        # Get number of updates/intents
        df = df.sort_values(by=['total_time']).reset_index().rename(
            columns={'total_time': 'time'})
        df[method] = df.index + 1
        df = df[df.columns.drop(list(df.filter(regex='index')))]
        return df

    for new_intent in df.new_intent.unique():
        time_df = df[df.new_intent == new_intent].drop(['new_intent'], axis=1)

        # For each method, get the time vs num_intents/updates DF
        cdf_df = pd.DataFrame()

        for method in time_df.method.unique():
            method_df = time_df[time_df.method == method]
            method_df = method_df.drop(['method'], axis=1)
            df1 = get_time_cdf_df(method_df, method)
            if cdf_df.empty:
                cdf_df = df1
            else:
                cdf_df = pd.merge(cdf_df, df1, how='outer', on='time')
            del df1
            del method_df

        time_df = cdf_df.sort_values(by='time').interpolate(
            limit_area='inside')
        del cdf_df
        columns = list(time_df.columns.drop(['time']))
        ax = time_df.plot(
            x='time',
            y=columns,
            kind='line',
            legend=False,
            xlabel='',
            ylabel='',
            rot=0,
            lw=LINE_WIDTH,
        )
        ax.grid(axis='both')
        ax.legend(bbox_to_anchor=(0.5, 1.1), ncol=len(columns), fontsize=14)
        ax.set_xscale('log')
        ax.set_xlabel('Time (seconds)', fontsize='x-small')
        ax.set_ylabel('Number of ' + ('intents' if new_intent else 'updates'),
                      fontsize='x-small')
        ax.tick_params(axis='both', which='both', labelsize='x-small')
        ax.tick_params(axis='y', which='minor', left=False, right=False)
        fig = ax.get_figure()
        fn = os.path.join(
            outDir,
            ('granularity.time.' +
             ('first-run.' if new_intent else 'update.') + network + '.pdf'))
        fig.savefig(fn)
        plt.close(fig)

    #
    # Memory
    #
    # Filter rows
    df = df[df.new_intent == True]
    # Filter columns
    df = df.drop(['new_intent', 'timestamp', 'total_time'], axis=1)

    def get_memory_df(df, method, extrapolate_factor=1):
        # Filter columns
        df = df.drop(['intent'], axis=1)
        df = df.rename(columns={'memory': method})
        # Sorting
        df = df.sort_values(by=[method])
        # Reset index for count
        df = df.reset_index().drop(['index'], axis=1)
        df['num_intents'] = df.index + 1
        # Extrapolate
        if extrapolate_factor > 1:
            df.num_intents *= extrapolate_factor
            df[method] *= extrapolate_factor
        return df

    # For each method, get the memory vs num_updates DF
    mem_df = pd.DataFrame()

    for method in df.method.unique():
        method_df = df[df.method == method]
        method_df = method_df.drop(['method'], axis=1)
        df1 = get_memory_df(method_df, method)
        if mem_df.empty:
            mem_df = df1
        else:
            mem_df = pd.merge(mem_df, df1, how='outer', on='num_intents')
        del df1
        del method_df

    network = reformat_string_for_filenames(network)
    columns = list(mem_df.columns.drop(['num_intents']))
    ax = mem_df.plot(
        x='num_intents',
        y=columns,
        kind='line',
        legend=False,
        xlabel='',
        ylabel='',
        rot=0,
        lw=LINE_WIDTH,
    )
    ax.grid(axis='both')
    ax.legend(bbox_to_anchor=(0.5, 1.1), ncol=len(columns), fontsize=14)
    ax.set_xlabel('Number of intents', fontsize='x-small')
    ax.set_ylabel('Memory (GiB)', fontsize='x-small')
    ax.tick_params(axis='both', which='both', labelsize='x-small')
    ax.tick_params(axis='x', which='minor', left=False, right=False)
    fig = ax.get_figure()
    fn = os.path.join(outDir,
                      ('granularity.memory.first-run.' + network + '.pdf'))
    fig.savefig(fn)
    plt.close(fig)


def plot_local_multi_intent(baseDir, outDir):
    df = pd.read_csv(os.path.join(baseDir, 'results.multi-intent.csv'))
    df = rewrite_values(df)

    for network in df.network.unique():
        net_df = df[df.network == network]
        plot_multi_intent_time(net_df, outDir, network)
        plot_multi_intent_memory(net_df, outDir, network)
        plot_multi_intent_granularity(net_df, outDir, network)


def plot_cluster_time(df, outDir, new_intent, num_nodes):
    # Filter columns
    df = df.drop([
        'platform', 'num_nodes', 'timestamp', 'intent', 'new_intent', 'memory'
    ],
                 axis=1)
    df = df.head(3000)  # Limit the numebr of intents/updates

    ##
    ## Time
    ##
    def get_time_cdf_df(df, col):
        df = df[col].to_frame()
        df = df.sort_values(by=[col]).reset_index().rename(
            columns={col: 'time'})
        df[col] = df.index + 1
        df = df[df.columns.drop(list(df.filter(regex='index')))]
        return df

    # For each method, get the time vs num_intents/updates DF
    cdf_df = pd.DataFrame()

    for col in list(df.columns):
        df1 = get_time_cdf_df(df, col)
        if cdf_df.empty:
            cdf_df = df1
        else:
            cdf_df = pd.merge(cdf_df, df1, how='outer', on='time')
        del df1

    df = cdf_df.sort_values(by='time').interpolate(limit_area='inside').rename(
        columns={
            'total_time': 'Total time',
            'intent_check_time': 'Intent check time',
            'model_build_time': 'Model build time',
            'disk_IO_time': 'Disk I/O',
        })
    del cdf_df

    ax = df.plot(
        x='time',
        y=['Disk I/O', 'Model build time', 'Intent check time', 'Total time'],
        kind='line',
        legend=False,
        xlabel='',
        ylabel='',
        rot=0,
        lw=LINE_WIDTH,
    )
    ax.grid(axis='both')
    ax.legend(bbox_to_anchor=(0.5, 1.1), ncol=4, fontsize=14)
    ax.set_xscale('log')
    ax.set_xlabel('Time (seconds)', fontsize='x-small')
    ax.set_ylabel('Number of intents', fontsize='x-small')
    ax.tick_params(axis='both', which='both', labelsize='x-small')
    ax.tick_params(axis='y', which='minor', left=False, right=False)
    fig = ax.get_figure()
    fn = os.path.join(outDir,
                      (str(num_nodes) + '-node-cluster.time.' +
                       ('first-run.' if new_intent else 'update.') + 'pdf'))
    fig.savefig(fn)
    plt.close(fig)


def plot_cluster_memory(df, outDir, new_intent):
    # Filter columns
    df = df.drop([
        'intent', 'new_intent', 'total_time', 'intent_check_time',
        'model_build_time', 'disk_IO_time'
    ],
                 axis=1)
    # Sorting
    df = df.sort_values(by=['num_nodes', 'timestamp', 'platform', 'memory'])
    # Get overall memory across nodes for each setup
    setup_dfs = list()
    for num_nodes in df.num_nodes.unique():
        setup_df = df[df.num_nodes == num_nodes].copy().reset_index()
        setup_df = setup_df[setup_df.columns.drop(
            list(setup_df.filter(regex='index')))]
        memories = pd.Series(dtype=float)
        overall_memories = []
        bootstrap_index = 0
        for index, row in setup_df.iterrows():
            memories[row['platform']] = row['memory']
            if len(memories) < num_nodes:
                bootstrap_index = index
            overall_memory = sum(memories)
            overall_memories.append(overall_memory)
        setup_df['overall_memory'] = overall_memories
        setup_df = setup_df.tail(-(bootstrap_index + 1))
        setup_df = setup_df.head(3000)  # Limit the numebr of intents/updates
        setup_df = setup_df.reset_index()
        setup_df['num_intents'] = setup_df.index + 1
        setup_df = setup_df.drop(['platform', 'timestamp', 'memory'], axis=1)
        setup_dfs.append(setup_df)
        del setup_df

    df = pd.concat(setup_dfs)
    df = df.pivot(
        index='num_intents', columns='num_nodes',
        values='overall_memory').reset_index().rename(columns={
            3: '3-node cluster',
            5: '5-node cluster'
        })
    columns = list(df.columns.drop(['num_intents']))
    ax = df.plot(
        x='num_intents',
        y=columns,
        kind='line',
        legend=False,
        xlabel='',
        ylabel='',
        rot=0,
        lw=LINE_WIDTH,
    )
    ax.grid(axis='both')
    ax.legend(bbox_to_anchor=(0.5, 1.1), ncol=len(columns), fontsize=14)
    ax.set_xlabel('Number of intents', fontsize='x-small')
    ax.set_ylabel('Memory (GiB)', fontsize='x-small')
    ax.tick_params(axis='both', which='both', labelsize='x-small')
    ax.tick_params(axis='x', which='minor', left=False, right=False)
    fig = ax.get_figure()
    fn = os.path.join(outDir,
                      ('cluster.memory.' +
                       ('first-run.' if new_intent else 'update.') + 'pdf'))
    fig.savefig(fn)
    plt.close(fig)


def plot_cloud_cluster(baseDir, outDir):
    df = pd.read_csv(os.path.join(baseDir, 'results.cluster.csv'))

    for new_intent in df.new_intent.unique():
        ni_df = df[df.new_intent == new_intent]
        for num_nodes in ni_df.num_nodes.unique():
            setup_df = ni_df[ni_df.num_nodes == num_nodes]
            plot_cluster_time(setup_df, outDir, new_intent, num_nodes)
        plot_cluster_memory(ni_df, outDir, new_intent)


def plot_cluster_e2e(baseDir, outDir):
    df = pd.read_csv(os.path.join(baseDir, 'results.cluster-e2e.csv'))

    df = df.pivot(index='num_intents', columns='num_nodes',
                  values='timestamp').reset_index().rename(columns={
                      3: '3-node cluster',
                      5: '5-node cluster'
                  })
    columns = list(df.columns.drop(['num_intents']))
    ax = df.plot(
        x='num_intents',
        y=columns,
        kind='line',
        figsize=(10, 8.0),
        legend=False,
        xlabel='',
        ylabel='',
        rot=0,
        lw=LINE_WIDTH,
    )
    ax.grid(axis='both')
    ax.legend(bbox_to_anchor=(0.5, 1.25),
              ncol=len(columns),
              columnspacing=0.7,
              fontsize=35)
    ax.set_xlabel('Number of intents', fontsize=40)
    ax.set_ylabel('End-to-end time (s)', fontsize=40)
    ax.tick_params(axis='both', which='both', labelsize=35)
    ax.tick_params(axis='x', which='minor', left=False, right=False)
    fig = ax.get_figure()
    fn = os.path.join(outDir, 'end-to-end.time.pdf')
    fig.savefig(fn)


def plot_widening_intents(df, outDir, network):
    network = reformat_string_for_filenames(network)
    # Filter rows
    df = df.loc[df.num_nodes == 9]
    df = df.loc[df.dist_scheme == 'dynamic-greedy']
    # Filter columns
    df = df.drop([
        'network', 'num_nodes', 'dist_scheme', 'num_intents',
        'avg_memory_per_node', 'rule_traversals', 'distinct_rules',
        'device_traversals', 'distinct_devices', 'device_reuse'
    ],
                 axis=1)
    # Sorting
    df = df.sort_values(by=['intent_width'])
    # Compute numbers of paths
    df['num_paths'] = df.intent_width**2
    df = df.drop(['intent_width'], axis=1)
    df = df.set_index('num_paths')
    # Rename column titles
    df = df.rename(
        columns={
            'peak_total_time': 'Peak time',
            'avg_time_per_intent': 'Average time',
            'overall_memory': 'Total memory',
            'peak_memory': 'Peak memory',
            'rule_reuse': 'Rule reuse',
        })

    ##
    ## Time
    ##
    ax = df.plot(
        # x='num_paths',
        y='Peak time',
        kind='line',
        figsize=(10, 8),
        legend=False,
        xlabel='',
        ylabel='',
        rot=0,
        lw=LINE_WIDTH,
    )
    df['Average time'].plot(
        secondary_y=True,
        mark_right=False,
        kind='line',
        legend=False,
        xlabel='',
        ylabel='',
        rot=0,
        lw=LINE_WIDTH,
    )
    ax.grid(axis='both')
    ax.legend(bbox_to_anchor=(0.08, 1.24), ncol=1, fontsize=34)
    ax.right_ax.legend(bbox_to_anchor=(0.75, 1.24), ncol=1, fontsize=34)
    ax.set_xlabel('Number of traversals per intent', fontsize=36)
    ax.set_ylabel('Peak time (seconds)', fontsize=36)
    ax.right_ax.set_ylabel('Average time (seconds)', fontsize=34)
    ax.tick_params(axis='both', which='both', labelsize=34)
    ax.right_ax.tick_params(axis='both', which='both', labelsize=34)
    ax.tick_params(axis='x', which='minor', left=False, right=False)
    ax.set_ylim(ymin=0)
    ax.right_ax.set_ylim(ymin=0)
    fig = ax.get_figure()
    fn = os.path.join(outDir, 'widening.time.' + network + '.pdf')
    fig.savefig(fn)

    ##
    ## Total memory
    ##
    ax = df.plot(
        # x='num_paths',
        y='Total memory',
        kind='line',
        figsize=(10, 8),
        legend=False,
        xlabel='',
        ylabel='',
        rot=0,
        lw=LINE_WIDTH,
    )
    df['Rule reuse'].plot(
        secondary_y=True,
        mark_right=False,
        kind='line',
        legend=False,
        xlabel='',
        ylabel='',
        rot=0,
        lw=LINE_WIDTH,
    )
    ax.grid(axis='both')
    ax.legend(bbox_to_anchor=(0.16, 1.24), ncol=1, fontsize=34)
    ax.right_ax.legend(bbox_to_anchor=(0.75, 1.24), ncol=1, fontsize=34)
    ax.set_xlabel('Number of traversals per intent', fontsize=36)
    ax.set_ylabel('Total memory (GiB)', fontsize=36)
    ax.right_ax.set_ylabel('Rule reuse', fontsize=36)
    ax.tick_params(axis='both', which='both', labelsize=34)
    ax.right_ax.tick_params(axis='both', which='both', labelsize=34)
    ax.tick_params(axis='x', which='minor', left=False, right=False)
    ax.set_ylim(ymin=0)
    ax.right_ax.set_ylim(ymin=0)
    fig = ax.get_figure()
    fn = os.path.join(outDir, 'widening.memory.' + network + '.pdf')
    fig.savefig(fn)


def plot_dist_schemes(df, outDir, printOnly=False):
    # For sorting values
    def values_compare(series):
        ids = {
            # networks
            'N1': 0,
            'N2': 1,
            'N3': 2,
            'N4': 3,
            'N5': 4,
            # distribution schemes
            'Hashing': 5,
            'K-medoids': 6,
            'Dynamic': 7,
            'None': 8,
        }
        return series.apply(lambda m: ids[m])

    # Metric labels (y-axes)
    metric_labels = {
        'peak_total_time': 'Peak time (seconds)',
        'avg_time_per_intent': 'Average time (seconds)',
        'overall_memory': 'Total memory (GiB)',
        'avg_memory_per_node': 'Average memory (GiB)',
        'peak_memory': 'Peak memory (GiB)',
        'rule_reuse': 'Rule reuse rate',
    }

    df = df.loc[(df.network != 'N5') | (df.intent_width != 1) |
                (df.num_nodes != 9) | (df.dist_scheme != 'dynamic-greedy')]
    df.loc[(df.network == 'N5') & (df.intent_width == 1) & (df.num_nodes == 9)
           & (df.dist_scheme == 'dynamic-hierarchical'),
           'dist_scheme'] = 'dynamic'
    df = df.loc[(df.network != 'N5') | (df.intent_width != 1) |
                (df.num_nodes != 9) | (df.dist_scheme != 'k-medoids-hashing')]
    df.loc[(df.network == 'N5') & (df.intent_width == 1) & (df.num_nodes == 9)
           & (df.dist_scheme == 'k-medoids'),
           'dist_scheme'] = 'k-medoids-hashing'

    # Filter rows
    df = df.loc[df.intent_width == 1]
    df = df.loc[(df.num_nodes == 9) | (df.num_nodes == 1)]
    df = df.loc[df.dist_scheme != 'dynamic-hierarchical']
    df = df.loc[df.dist_scheme != 'k-medoids']
    # Filter columns
    df = df.drop([
        'intent_width', 'num_nodes', 'num_intents', 'rule_traversals',
        'distinct_rules', 'device_traversals', 'distinct_devices',
        'device_reuse'
    ],
                 axis=1)
    # Rename values
    df = df.replace('hashing', 'Hashing')
    df = df.replace('k-medoids-hashing', 'K-medoids')
    df = df.replace('dynamic', 'Dynamic')
    df = df.replace('dynamic-greedy', 'Dynamic')
    df = df.replace('none', 'None')
    # Sorting
    df = df.sort_values(by=['network', 'dist_scheme'], key=values_compare)

    if printOnly:
        pd.set_option("display.precision", 3)
        print(df)
        return

    for metric in df.columns.drop(['network', 'dist_scheme']):
        # Filter columns
        metric_df = df.drop(df.columns.drop(['network', 'dist_scheme',
                                             metric]),
                            axis=1)
        dist_schemes = metric_df.dist_scheme.unique()
        metric_df = metric_df.pivot(index='network',
                                    columns='dist_scheme',
                                    values=metric)  #.reset_index()
        ax = metric_df.plot(
            y=dist_schemes,
            kind='bar',
            figsize=(10, 8),
            legend=False,
            width=0.87,
            xlabel='',
            ylabel='',
            rot=0,
        )
        ax.grid(axis='y')
        ax.legend(bbox_to_anchor=(0.5, 1.4),
                  ncol=2,
                  fontsize=35,
                  columnspacing=0.6)
        # ax.set_yscale('log')
        ax.set_ylabel(metric_labels[metric], fontsize=40)
        ax.tick_params(axis='both', which='both', labelsize=35)
        ax.tick_params(axis='x',
                       which='both',
                       top=False,
                       bottom=False,
                       labelsize=40)
        fig = ax.get_figure()
        fn = os.path.join(outDir, 'dist-schemes.' + metric + '.pdf')
        fig.savefig(fn)


def plot_placement(baseDir, outDir):
    df = pd.read_csv(os.path.join(baseDir, 'results.placement.csv'))
    df = rewrite_values(df)

    plot_dist_schemes(df, outDir)
    plot_dist_schemes(df, outDir, printOnly=True)
    for network in df.network.unique():
        net_df = df[df.network == network]
        plot_widening_intents(net_df, outDir, network)


def plot_perf_vs_tenants(df, outDir):

    def _plot(df, outDir, nproc, drop, inv):
        # Filter rows
        df = df[df.model_only == False]
        # Filter columns
        df = df.drop([
            'model_only', 'num_nodes', 'num_links', 'num_updates',
            'independent_cec'
        ],
                     axis=1)
        # Sorting
        df = df.sort_values(by=['tenants', 'updates'])
        # Change units
        df['inv_time'] /= 1e6  # usec -> sec
        df['inv_memory'] /= 1024  # KiB -> MiB
        # Rename column titles
        # df = df.rename(columns={'inv_time', 'Time'})
        # Rename values
        df.loc[df['updates'] == 0, 'updates'] = 'None'
        df.loc[df['tenants'] == df['updates'] * 2, 'updates'] = 'Half-tenant'
        df.loc[df['tenants'] == df['updates'], 'updates'] = 'All-tenant'

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
        fn = os.path.join(outDir, ('06.multi-tenant.inv-' + str(inv) + '.' +
                                   str(nproc) + '-procs.' + drop + '.pdf'))
        fig.savefig(fn, bbox_inches='tight')
        plt.close(fig)

    for nproc in df.procs.unique():
        nproc_df = df[df.procs == nproc].drop(['procs'], axis=1)

        for drop in nproc_df.drop_method.unique():
            d_df = nproc_df[nproc_df.drop_method == drop].drop(['drop_method'],
                                                               axis=1)

            for inv in d_df.invariant.unique():
                inv_df = d_df[d_df.invariant == inv].drop(['invariant'],
                                                          axis=1)
                _plot(inv_df, outDir, nproc, drop, inv)


def plot_perf_vs_nprocs(df, outDir):
    pass
    # TODO


def plot_stats(invDir, outDir, compare_model=False):
    df = pd.read_csv(os.path.join(invDir, 'stats.csv'))

    plot_perf_vs_tenants(df, outDir)
    plot_perf_vs_nprocs(df, outDir)


def plot_latency(invDir, outDir):
    df = pd.read_csv(os.path.join(invDir, 'lat.csv'))
    # TODO


def main():
    parser = argparse.ArgumentParser(description='Plotting for Neo')
    parser.add_argument('-t',
                        '--target',
                        help='Plot target',
                        type=str,
                        action='store',
                        required=True)
    parser.add_argument('--compare-model',
                        help='Compare Neo with model-based approach',
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

    plot_stats(targetDir, outDir, args.compare_model)
    plot_latency(targetDir, outDir)


if __name__ == '__main__':
    main()
