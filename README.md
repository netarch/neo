# Neo

> [!IMPORTANT]
> For artifact evaluation, please start at
> [Artifact Evaluation](/doc/artifact-evaluation.md).

Neo is a network testing tool that implements concolic network execution. It
combines symbolic model checking and container-based emulation techniques to
achieve high-quality network-wide testing for networks that involve stateful
software components (e.g., software network functions, middleboxes).

<!--toc:start-->
- [Neo](#neo)
  - [Dependencies](#dependencies)
  - [Build and install Neo](#build-and-install-neo)
  - [Basic usage](#basic-usage)
    - [Running the examples](#running-the-examples)
    - [Running through run.sh](#running-through-runsh)
    - [Running Neo directly](#running-neo-directly)
  - [Understanding the output](#understanding-the-output)
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
# elapsed time: 1 min 19.20 sec
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
# elapsed time: 8 min 42.43 sec (for the first time)
# peak memory:  413 MiB         (for the first time)
```

> [!NOTE]
> Running this step for the first time may take a while to build all vcpkg
> dependencies. Subsequent invocations should finish within a few seconds.

Once the configurationn succeeds, please run the following to build Neo.

```sh
$ ./scripts/build.sh

# ---[ Ubuntu 24.04 VM with 16 core vCPU and 32 GiB RAM ]---
# elapsed time: 22.80 sec
# peak memory:  324 MiB
```

When the build completes, the result will be a binary executable at
`./build/neo`. If you have configured to build unit tests previously, you can
optionally run the tests with the following command.

```sh
$ ./scripts/test.sh

# ---[ Ubuntu 24.04 VM with 16 core vCPU and 32 GiB RAM ]---
# elapsed time: ??
# peak memory:  ?? MiB
```

Afterward, you can install Neo to the system paths by running the following
command, or you could also just run the binary from the build directory. The
example command below installs the binary at `/usr/bin/neo`.

```sh
$ sudo ./scripts/install.sh --prefix /usr

# ---[ Ubuntu 24.04 VM with 16 core vCPU and 32 GiB RAM ]---
# elapsed time: 0.05 sec
# peak memory:  7 MiB
```

## Basic usage

TODO: asdf

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

### Running the examples

The examples can be found in `examples/` directory. You can test the examples by
running `run.sh` included in the example directories, or optionally by feeding
the configuration files directly to Neo.

### Running through run.sh

You can try the examples by running `run.sh` in each example directory. This may trigger Neo multiple times with different configurations. The output can be found in `result` directory inside the example directory. For instance, you can try `00-reverse-path-filtering` by executing:
```sh
examples/00-reverse-path-filtering/run.sh
```
The output can be found in `examples/00-reverse-path-filtering/results`.

### Running Neo directly

If you want to have more control on the execution, you can run the examples by directly feeding Neo the configuration files. Each example either contains TOML configuration files or contains a confgen.py file which generates a TOML file. For instance, `00-reverse-path-filtering` contains two different TOML files, `network.fault.toml` and `network.toml`. Each TOML file corresponds to a network configuration to be tested as well as the invariants of interest. To run Neo with `examples/00-reverse-path-filtering/network.toml` as the input, execute:
```sh
sudo neo -afj8 -i examples/00-reverse-path-filtering/network.toml -o output

# ---[ Ubuntu 24.04 VM with 16 core vCPU and 32 GiB RAM ]---
# elapsed time: 3.74 sec
# peak memory:  7 MiB
```

If the example contains `confgen.py` instead, the TOML file must be generated using the python script before running Neo. Each `confgen.py` for different examples expects different parameters, such as the number of subnets, number of hosts in each subnet, etc. The expected parameters can be found by running `confgen.py` with `--help` flag. Once the TOML file is generated, you can run Neo with the generated TOML file. For instance, to run `01-subnet-isolation` with two subnets and two hosts per subnet, execute:
```sh
python3 examples/01-subnet-isolation/confgen.py -s 2 -H 2 > examples/01-subnet-isolation/network.toml
sudo neo -afj8 -i examples/01-reverse-path-filtering/network.toml -o output
```
The output can be found in `output/` directory in the Neo home directory.

## Understanding the output

> [!NOTE]
> Note that root privilege is needed because the process needs to create virtual
> interfaces, inject packets, manipulate namespaces, and modify routing table
> entries within respective namespaces.
