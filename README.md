Plankton-neo
============

[![Travis](https://img.shields.io/travis/com/netarch/neo.svg)](https://travis-ci.com/netarch/neo)
[![GitHub workflow](https://github.com/netarch/neo/workflows/test/badge.svg)](https://github.com/netarch/neo/actions)
[![Codecov](https://img.shields.io/codecov/c/github/netarch/neo.svg)](https://codecov.io/gh/netarch/neo)
[![GitHub repo size](https://img.shields.io/github/repo-size/netarch/neo.svg)](https://github.com/netarch/neo)

Plankton-neo is a network testing tool based on the Plankton verification
framework. It combines explicit-state model checking and emulation techniques to
achieve high-coverage testing for softwarized networks with middlebox
components.

#### Table of Contents

- [Environment setup](#environment-setup)
    - [CMake](#cmake)
    - [SPIN](#spin)
- [Build and install Plankton-neo](#build-and-install-plankton-neo)
- [Usage](#usage)


## Environment setup

The following dependencies are needed for Plankton-neo.

- make
- cmake (>= 3.12)
- spin (>= 6.5.2)
- libnet
- modern C and C++ compilers (GCC or Clang)

`depends/setup.sh` can be used to set up the development environment, but
additional packages may be installed and it may overwrite existing packages in
the system. Some platforms may not yet be supported. Pull requests and issues
are appreciated.

### CMake

Please check that the cmake version is at least 3.12 with `cmake --version`. If
`depends/setup.sh` was used, the correct version should be installed. Otherwise,
you can install a newer version from the official pre-built releases. For
example,

```sh
$ curl -LO "https://github.com/Kitware/CMake/releases/download/v3.19.6/cmake-3.19.6-Linux-x86_64.tar.gz"
$ tar xf cmake-3.19.6-Linux-x86_64.tar.gz
$ sudo rsync -av cmake-3.19.6-Linux-x86_64/* /usr/
```

### SPIN

If `depends/setup.sh` was used, the correct version should be installed. But if
the spin version is older than 6.5.2 (`spin -V`), please install the latest
version of SPIN from its [official
repository](https://github.com/nimble-code/Spin).

```sh
$ curl -LO "https://github.com/nimble-code/Spin/archive/version-6.5.2.tar.gz"
$ tar xf version-6.5.2.tar.gz
$ cd Spin-version-6.5.2/
$ make -C Src -j$(nproc)
$ gzip -9c Man/spin.1 > Man/spin.1.gz
$ sudo install -Dm 755 Src/spin /usr/bin/spin
$ sudo install -Dm 644 Man/spin.1.gz /usr/share/man/man1/spin.1.gz
$ sudo install -Dm 644 Src/LICENSE /usr/share/licenses/spin/LICENSE
```

## Build and install Plankton-neo

```sh
$ cd neo
$ git submodule update --init --recursive
$ cmake -B build -S . -DCMAKE_BUILD_TYPE=Release    # Release or Debug
$ cmake --build build -j$(nproc)
$ sudo cmake --install build
```

## Usage

```
Usage: neo [OPTIONS] -i <file> -o <dir>
Options:
    -h, --help           print this help message
    -a, --all            verify all ECs after violation
    -f, --force          remove pre-existent output directory
    -l, --latency        measure packet injection latencies
    -j, --jobs <N>       number of parallel tasks
    -e, --emulations <N> maximum number of emulation instances
    -i, --input <file>   network configuration file
    -o, --output <dir>   output directory
```

Examples:

```sh
$ sudo neo -fj8 -i examples/00-reverse-path-filtering/network.toml -o output
```

Note that root privilege is needed because the process needs to create virtual
interfaces, inject packets, and modify routing table entries within respective
network namespaces.
