# Neo

Neo is a network testing tool that implements concolic network execution. It
combines symbolic model checking and container-based emulation techniques to
achieve high-quality network-wide testing for networks that involve stateful
software components (e.g., software network functions, middleboxes).

<!--toc:start-->
- [Neo](#neo)
  - [Dependencies](#dependencies)
  - [Build and install Neo](#build-and-install-neo)
  - [Basic usage](#basic-usage)
  - [Understanding the output](#understanding-the-output)
  - [Batch testing with run.sh](#batch-testing-with-runsh)
<!--toc:end-->

## Dependencies

We provide a convenience script `depends/setup.sh` to automatically set up all
dependencies required to build and run Neo. Currently the script only supports
Arch Linux and Ubuntu 24.04. Though Neo should also work on other Linux
distributions, we don't provide official support. Please read `depends/setup.sh`
for all dependency details if you want to install them manually.

After cloning the repository, enter the directory and run:

```sh
$ ./depends/setup.sh

# ---[ Ubuntu 24.04 VM with 16 core vCPU and 32 GiB RAM ]---
# elapsed time: 2 min 20.32 sec
# peak memory:  47 MiB
```

> [!IMPORTANT]
> After running the setup script, it is crucial to logout and re-login again for
> the new group configuration to take effect. Otherwise, there may be permission
> errors from Docker. Alternatively, you can also reboot the OS entirely.

## Build and install Neo

To configure Neo, please run the following command. The option `-t | --test` is
used to build the unit tests. `-h | --help` may be used to see all available
options.

```sh
$ ./scripts/configure.sh -t

# ---[ Ubuntu 24.04 VM with 16 core vCPU and 32 GiB RAM ]---
# elapsed time: 4 min 45.73 sec (for the first time)
# peak memory:  413 MiB         (for the first time)
```

> [!NOTE]
> Running this step for the first time may take a while to build all vcpkg
> dependencies. Subsequent invocations should finish within a few seconds.

Once the configurationn succeeds, please run the following to build Neo.

```sh
$ ./scripts/build.sh

# ---[ Ubuntu 24.04 VM with 16 core vCPU and 32 GiB RAM ]---
# elapsed time: 23.75 sec
# peak memory:  325 MiB
```

When the build completes, the result will be a binary executable at
`./build/neo`. If you have configured to build unit tests previously, you can
optionally run the tests with the following command.

```sh
$ ./scripts/test.sh

# ---[ Ubuntu 24.04 VM with 16 core vCPU and 32 GiB RAM ]---
# elapsed time: 26.01 sec
# peak memory:  7 MiB
```

Afterward, you can install Neo to system paths by running the following command,
or you could also run the binary from the build directory. The example command
below installs the binary at `/usr/bin/neo`.

```sh
$ sudo ./scripts/install.sh --prefix /usr

# ---[ Ubuntu 24.04 VM with 16 core vCPU and 32 GiB RAM ]---
# elapsed time: 0.07 sec
# peak memory:  7 MiB
```

## Basic usage

Here we explain the basic usage of Neo with a simple example
`00-reverse-path-filtering`, assuming the compiled program is located at
`./build/neo`. To run the first example, please run the following commands:

```sh
$ sudo ./build/neo -fj8 -i examples/00-reverse-path-filtering/network.toml -o examples/00-reverse-path-filtering/output

# ---[ Ubuntu 24.04 VM with 16 core vCPU and 32 GiB RAM ]---
# elapsed time: 4.04 sec
# peak memory:  7 MiB

$ sudo chown -R "$(id -u):$(id -g)" examples/00-reverse-path-filtering/output

# ---[ Ubuntu 24.04 VM with 16 core vCPU and 32 GiB RAM ]---
# elapsed time: 0.01 sec
# peak memory:  7 MiB
```

> [!NOTE]
> Note that root privilege is needed because Neo needs to create virtual
> interfaces, manipulate namespaces, and modify routing table entries within the
> respective namespaces.

The `-i` and `-o` options specify the input configuration file and the output
directory, respectively. The `-f` option forcefully removes the specified output
directory if it already exists, and the `-j` option allows specifying the
maximum number of parallel tasks. `-h` can be used to see all available options.

```
$ ./build/neo -h
Neo options:
  -h [ --help ]                Show help message
  -a [ --all ]                 Verify all ECs after violation
  -f [ --force ]               Remove output dir if exists
  -p [ --parallel-invs ]       Allow verifying invariants in parallel (default:
                               disabled). This is mostly used for narrow
                               invariants who only have one EC to check.
  -j [ --jobs ] arg (=1)       Max number of parallel tasks [default: 1]
  -e [ --emulations ] arg (=0) Max number of emulations
  -d [ --drop ] arg (=timeout) Drop detection method: ['timeout', 'dropmon',
                               'ebpf']
  -i [ --input ] arg           Input configuration file
  -o [ --output ] arg          Output directory
```

## Understanding the output

The command output of running the first example would look something similar to
the following. It shows a summary of the invariants being tested, the number of
connection ECs (CECs) specified within each invariant, and the end-to-end
performance of testing all the specified invariants.

```
[1721770394.013361] [66929] [II] Parsing configuration file .../neo/examples/00-reverse-path-filtering/network.toml
[1721770394.267384] [66929] [II] Loaded 9 nodes
[1721770394.267445] [66929] [II] Loaded 9 links
[1721770394.268731] [66929] [II] Loaded 3 invariants
[1721770395.290439] [66929] [II] Initial ECs: 28
[1721770395.290514] [66929] [II] Initial ports: 1
[1721770395.291251] [67384] [II] ====================
[1721770395.291323] [67384] [II] 1. Verifying invariant Reachability (O): [ server ]
[1721770395.291331] [67384] [II] Connection ECs: 3
[1721770396.101593] [68264] [II] ====================
[1721770396.101648] [68264] [II] 2. Verifying invariant Reachability (X): [ h1 h2 h3 s1 r1 fw r2 r3 ]
[1721770396.101655] [68264] [II] Connection ECs: 12
[1721770397.029864] [69667] [II] ====================
[1721770397.029926] [69667] [II] 3. Verifying invariant ReplyReachability (O): [ server ]
[1721770397.029933] [69667] [II] Connection ECs: 3
[1721770397.909497] [66929] [II] ====================
[1721770397.909542] [66929] [II] Time: 2618897 usec
[1721770397.909551] [66929] [II] Peak memory: 157148 KiB
[1721770397.909559] [66929] [II] Current memory: 10496 KiB
```

The structure of the output directory looks like the following. Each `<pid>.log`
within the invariant directories captures the entire depth-first traversal of
the state space given the initial CEC and the network specification.

```
output/                         <-- output directory specified by `-o`
  |
  +-- main.log                  <-- top-level log, same as command output
  |
  +-- 1/                        <-- invariant #1
  |   `-- <pid>.log             <-- Neo and SPIN testing output for each CEC
  |   `-- <pid>.stats.csv       <-- perf stats for each CEC
  |   `-- invariant.stats.csv   <-- invariant-level aggregated perf stats
  |
  +-- 2/                        <-- invariant #2
  |   `-- <pid>.log             <-- Neo and SPIN testing output for each CEC
  |   `-- <pid>.stats.csv       <-- perf stats for each CEC
  |   `-- invariant.stats.csv   <-- invariant-level aggregated perf stats
  |
  `-- 3/                        <-- invariant #3
      `-- <pid>.log             <-- Neo and SPIN testing output for each CEC
      `-- <pid>.stats.csv       <-- perf stats for each CEC
      `-- invariant.stats.csv   <-- invariant-level aggregated perf stats
```

## Batch testing with run.sh

To automate testing and to reproduce the experiments from the paper, many
examples in the `examples/` directory provide a `run.sh` script that tests the
given networks with different combinations of parameters and the input
configuration files.

Please see [Artifact Evaluation](doc/artifact-evaluation.md) for a complete
description of how to reproduce the experiments and process the results.
