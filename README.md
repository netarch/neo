Neo
===

[![GitHub workflow](https://github.com/netarch/neo/workflows/test/badge.svg)](https://github.com/netarch/neo/actions)
[![Codecov](https://img.shields.io/codecov/c/github/netarch/neo.svg)](https://codecov.io/gh/netarch/neo)
[![GitHub repo size](https://img.shields.io/github/repo-size/netarch/neo.svg)](https://github.com/netarch/neo)

Neo is a network testing tool based on the Plankton verification framework. It
combines explicit-state model checking and emulation techniques to achieve
high-coverage testing for softwarized networks with middlebox components.

#### Table of Contents

- [Neo](#neo)
      - [Table of Contents](#table-of-contents)
  - [Environment setup](#environment-setup)
  - [Build Neo](#build-neo)
  - [Usage](#usage)


## Environment setup

The following dependencies are needed for Neo.

- modern C and C++ compilers
- cmake (>= 3.13)
- libnet
- libnl
- boost
- curl
- pcapplusplus
- [spin](https://github.com/nimble-code/Spin)
- docker
- Linux (>= 5.4)

`depends/setup.sh` can be used to automatically set up the environment, but it
only supports Arch Linux and Ubuntu at the moment. Pull requests are
appreciated.

Since Neo requires Linux version to be at least 5.4 for [per-packet drop
monitoring](https://github.com/torvalds/linux/commit/ca30707dee2bc8bc81cfd8b4277fe90f7ca6df1f),
and Ubuntu 18 ships Linux 4.15 by default, it is recommended to use at least
Ubuntu 20.04. Otherwise newer kernel versions will need to be manually
installed.

## Build and install Neo

```sh
$ cd neo
$ git submodule update --init --recursive
$ rm -rf build
$ cmake -B build -S . -DCMAKE_BUILD_TYPE=Release
$ cmake --build build -j $(nproc)
```

Optionally you can install Neo to system paths if desired.

```sh
$ sudo DESTDIR=/ cmake --install build --prefix /usr
```

## Usage

```
Neo options:
  -h [ --help ]                Show help message
  -a [ --all ]                 Verify all ECs after violation
  -f [ --force ]               Remove output dir if exists
  -d [ --dropmon ]             Use drop_monitor for packet drops
  -j [ --jobs ] arg (=1)       Max number of parallel tasks [default: 1]
  -e [ --emulations ] arg (=0) Max number of emulations
  -i [ --input ] arg           Input configuration file
  -o [ --output ] arg          Output directory
```

Examples:

```sh
$ sudo neo -afj8 -i examples/00-reverse-path-filtering/network.toml -o output
```

> **Note** <br/>
> Note that root privilege is needed because the process needs to create virtual
> interfaces, inject packets, manipulate namespaces, and modify routing table
> entries within respective namespaces.
