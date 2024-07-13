Neo
===

[![GitHub workflow](https://github.com/netarch/neo/workflows/test/badge.svg)](https://github.com/netarch/neo/actions)
[![Codecov](https://img.shields.io/codecov/c/github/netarch/neo.svg)](https://app.codecov.io/gh/netarch/neo)
[![GitHub repo size](https://img.shields.io/github/repo-size/netarch/neo.svg)](https://github.com/netarch/neo)

Neo is a network testing tool based on the Plankton verification framework. It
combines explicit-state model checking and emulation techniques to achieve
high-coverage testing for softwarized networks with middlebox components.

## Table of Contents

- [Neo](#neo)
  - [Table of Contents](#table-of-contents)
  - [Environment setup](#environment-setup)
  - [Build and install Neo](#build-and-install-neo)
  - [Usage](#usage)
  - [Running the examples]
  - [Understanding the output]


## Environment setup

The following dependencies are needed for Neo.

- Clang
- LLVM
- libnl
- libelf
- zlib
- [spin](https://github.com/nimble-code/Spin)
- Docker

`depends/setup.sh` can be used to automatically set up the environment, but it
only supports Arch Linux and Ubuntu at the moment. Pull requests are
appreciated.

After running `depends/setup.sh`, please `logout` and log in again in order to apply the system group configuration modified by the script. If you install the dependencies manually, you can configure the group by running 
```sh
sudo gpasswd -a $USER docker
logout
```
and logging in again.

Since Neo requires Linux version to be at least 5.4 for [per-packet drop
monitoring](https://github.com/torvalds/linux/commit/ca30707dee2bc8bc81cfd8b4277fe90f7ca6df1f),
and Ubuntu 18 ships Linux 4.15 by default, it is recommended to use at least
Ubuntu 20.04. Otherwise newer kernel versions will need to be manually
installed.

## Build and install Neo

```sh
cd neo
./scripts/configure.sh
./scripts/build.sh
```

The executable will be at `build/neo`.

Optionally you can install Neo to system
paths if desired.

```sh
sudo DESTDIR=/ ./scripts/install.sh --prefix /usr
```

## Usage

```
Neo options:
  -h [ --help ]                Show help message
  -a [ --all ]                 Verify all ECs after violation
  -f [ --force ]               Remove output dir if exists
  -j [ --jobs ] arg (=1)       Max number of parallel tasks [default: 1]
  -e [ --emulations ] arg (=0) Max number of emulations
  -d [ --drop ] arg (=timeout) Drop detection method: ['timeout', 'dropmon',
                               'ebpf']
  -i [ --input ] arg           Input configuration file
  -o [ --output ] arg          Output directory
```

##Running the examples
The examples can be found in `examples/` directory. You can test the examples by running `run.sh` included in the example directories, or optionally by feeding the configuration files directly to Neo.

#Running through run.sh
You can try the examples by running `run.sh` in each example directory. This may trigger Neo multiple times with different configurations. The output can be found in `result` directory inside the example directory. For instance, you can try `00-reverse-path-filtering` by executing:
```sh
examples/00-reverse-path-filtering/run.sh
```
The output can be found in `examples/00-reverse-path-filtering/results`.

#Running Neo directly
If you want to have more control on the execution, you can run the examples by directly feeding Neo the configuration files. Each example either contains TOML configuration files or contains a confgen.py file which generates a TOML file. For instance, `00-reverse-path-filtering` contains two different TOML files, `network.fault.toml` and `network.toml`. Each TOML file corresponds to a network configuration to be tested as well as the invariants of interest. To run Neo with `examples/00-reverse-path-filtering/network.toml` as the input, execute:
```sh
sudo neo -afj8 -i examples/00-reverse-path-filtering/network.toml -o output
```

If the example contains `confgen.py` instead, the TOML file must be generated using the python script before running Neo. Each `confgen.py` for different examples expects different parameters, such as the number of subnets, number of hosts in each subnet, etc. The expected parameters can be found by running `confgen.py` with `--help` flag. Once the TOML file is generated, you can run Neo with the generated TOML file. For instance, to run `01-subnet-isolation` with two subnets and two hosts per subnet, execute:
```sh
python3 examples/01-subnet-isolation/confgen.py -s 2 -H 2 > examples/01-subnet-isolation/network.toml
sudo neo -afj8 -i examples/01-reverse-path-filtering/network.toml -o output
```
The output can be found in `output/` directory in the Neo home directory. 

##Understanding the output


> **Note** <br/>
> Note that root privilege is needed because the process needs to create virtual
> interfaces, inject packets, manipulate namespaces, and modify routing table
> entries within respective namespaces.
